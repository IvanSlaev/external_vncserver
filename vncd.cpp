/*
droid vnc server - Android VNC server
Copyright (C) 2009 Jose Pereira <onaips@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "common.h"
#include "log.h"
#include "flinger.h"
#include "clipboard.h"
#include "input.h"

extern "C" {
    #include "libvncserver/scale.h"
    #include "rfb/rfb.h"
    #include "rfb/rfbregion.h"
    #include "rfb/keysym.h"
}

#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#define CONCAT3(a,b,c) a##b##c
#define CONCAT3E(a,b,c) CONCAT3(a,b,c)

//  port 5900 is bound natively in some Android devices
int VNC_PORT = 5901;
char* VNC_PASSWD_FILE = "/data/vnc/password.bin";

unsigned int* cmpbuf;
unsigned int* vncbuf;

static rfbScreenInfoPtr vncscr;

uint32_t idle = 0;
uint32_t standby = 1;
uint16_t rotation = 0;
uint16_t scaling = 100;
uint8_t display_rotate_180 = 0;

// reverse connection
char *rhost = NULL;
int rport = 5500;

screenFormat screenformat;
void (*update_screen)(void) = NULL;

#define OUT 8
#include "screen/updateScreen.h"
#undef OUT

#define OUT 16
#include "screen/updateScreen.h"
#undef OUT

#define OUT 32
#include "screen/updateScreen.h"
#undef OUT

void setIdle(int i)
{
    idle = i;
}

void closeVncServer(int signo)
{
    L("Cleaning up for signo %d...\n", signo);

    closeFlinger();
    cleanupInput();
    exit(0);
}

void clientGone(rfbClientPtr cl)
{
    L("Client disconnected\n");
}

enum rfbNewClientAction clientHook(rfbClientPtr cl)
{
    if (scaling != 100)
    {
        int w = vncscr->width * scaling / 100;
        int h = vncscr->height * scaling / 100;

        L("Scaling to w=%d, h=%d\n", w, h);
        rfbScalingSetup(cl, w, h);
    }

    L("Client connected: %s\n", cl->host);
    cl->clientGoneHook = (ClientGoneHookPtr) clientGone;

    return RFB_CLIENT_ACCEPT;
}

void setClipboardText(char* str, int len, struct _rfbClientRec* cl)
{
    L("Updating local clipboard with remote text\n");
    setClipboard(len, str);
}

void setTextChat(struct _rfbClientRec* cl, int len, char *str)
{
    L("Text chat: %s\n", str);
}

void initVncServer(int argc, char **argv)
{
	vncbuf = (unsigned int*) calloc(screenformat.width * screenformat.height, screenformat.bitsPerPixel/CHAR_BIT);
	cmpbuf = (unsigned int*) calloc(screenformat.width * screenformat.height, screenformat.bitsPerPixel/CHAR_BIT);

	if (rotation==0 || rotation==180)
		vncscr = rfbGetScreen(&argc, argv, screenformat.width , screenformat.height, 0 /* not used */ , 3,	screenformat.bitsPerPixel/CHAR_BIT);
	else
		vncscr = rfbGetScreen(&argc, argv, screenformat.height, screenformat.width, 0 /* not used */ , 3,	screenformat.bitsPerPixel/CHAR_BIT);

	assert(vncbuf != NULL);
	assert(cmpbuf != NULL);
	assert(vncscr != NULL);

	vncscr->desktopName = "emteria.OS";
	vncscr->frameBuffer = (char*) vncbuf;
	vncscr->port = VNC_PORT;
	vncscr->newClientHook = (rfbNewClientHookPtr) clientHook;
	vncscr->kbdAddEvent = keyEvent;
	vncscr->ptrAddEvent = ptrEvent;
	vncscr->setXCutText = setClipboardText;
	vncscr->setTextChat = setTextChat;

    if (access(VNC_PASSWD_FILE, F_OK) != -1)
    {
        L("Using encrypted password file\n");
        vncscr->authPasswdData = VNC_PASSWD_FILE;
    }

//  vncscr->httpEnableProxyConnect = TRUE;
    vncscr->httpDir = "webclients/";
    vncscr->sslcertfile = "self.pem";

	vncscr->serverFormat.redShift = screenformat.redShift;
	vncscr->serverFormat.greenShift = screenformat.greenShift;
	vncscr->serverFormat.blueShift = screenformat.blueShift;

	vncscr->serverFormat.redMax = (( 1 << screenformat.redMax) -1);
	vncscr->serverFormat.greenMax = (( 1 << screenformat.greenMax) -1);
	vncscr->serverFormat.blueMax = (( 1 << screenformat.blueMax) -1);

	vncscr->serverFormat.trueColour = TRUE;
	vncscr->serverFormat.bitsPerPixel = screenformat.bitsPerPixel;

	vncscr->alwaysShared = TRUE;
	vncscr->handleEventsEagerly = TRUE;
	vncscr->deferUpdateTime = 5;

	rfbInitServer(vncscr);

	// assign update_screen depending on bpp
	if (vncscr->serverFormat.bitsPerPixel == 32)
	{
		update_screen = &CONCAT2E(update_screen_,32);
	}
	else if (vncscr->serverFormat.bitsPerPixel == 16)
	{
		update_screen = &CONCAT2E(update_screen_,16);
	}
	else if (vncscr->serverFormat.bitsPerPixel == 8)
	{
		update_screen = &CONCAT2E(update_screen_, 8);
	}
	else
	{
		L("Unsupported pixel depth: %d\n", vncscr->serverFormat.bitsPerPixel);
		closeVncServer(-1);
	}

	/* Mark as dirty since we haven't sent any updates at all yet. */
	rfbMarkRectAsModified(vncscr, 0, 0, vncscr->width, vncscr->height);
}

void rotate(int value)
{
	L("rotate()\n");

	if (value == -1 ||
		((value == 90 || value == 270) && (rotation == 0 || rotation == 180)) ||
		((value == 0 || value == 180) && (rotation == 90 || rotation == 270))) {
			int h = vncscr->height;
			int w = vncscr->width;

			vncscr->width = h;
			vncscr->height = w;
			vncscr->paddedWidthInBytes = h * screenformat.bitsPerPixel / CHAR_BIT;

			rfbClientPtr cl;
			rfbClientIteratorPtr iterator = rfbGetClientIterator(vncscr);
			while ((cl = rfbClientIteratorNext(iterator)) != NULL) {
				cl->newFBSizePending = 1;
			}
	}

	if (value == -1) {
		rotation += 90;
	} else {
		rotation = value;
	}
	rotation %= 360;

	rfbMarkRectAsModified(vncscr, 0, 0, vncscr->width, vncscr->height);
}

void extractReverseHostPort(char *str)
{
	int len = strlen(str);
	char *p;

	/* copy in to host */
	rhost = (char *) malloc(len+1);
	if (!rhost) {
		L("reverse_connect: could not malloc string %d\n", len);
		exit(-1);
	}

	strncpy(rhost, str, len);
	rhost[len] = '\0';

	/* extract port, if any */
	if ((p = strrchr(rhost, ':')) != NULL) {
		rport = atoi(p+1);
		if (rport < 0)
		{
			rport = -rport;
		} else if (rport < 20) {
			rport = 5500 + rport;
		}
		*p = '\0';
	}
}

void printUsage(char **argv)
{
	L("\nvncserver [parameters]\n"
		"-r <rotation>\t- Screen rotation (degrees) (0,90,180,270)\n"
		"-R <host:port>\t- Host for reverse connection\n"
		"-s <scale>\t- Scale percentage (20,30,50,100,150)\n"
		"-h\t\t- Print this help\n"
		"-v\t\t- Output version\n"
		"\n");
}

int main(int argc, char **argv)
{
    // pipe signals
    signal(SIGINT, closeVncServer);
    signal(SIGKILL, closeVncServer);
    signal(SIGILL, closeVncServer);

    if (argc > 1)
    {
		int i=1;
		int r;
		while (i < argc)
		{
			if (*argv[i] == '-')
			{
				switch(*(argv[i] + 1))
				{
					case 'h':
						printUsage(argv);
						exit(0);
						break;
					case 'P':
						i++;
						VNC_PORT = atoi(argv[i]);
						break;
					case 'r':
						i++;
						r = atoi(argv[i]);
						if (r==0 || r==90 || r==180 || r==270) { rotation = r; }
						L("rotating to %d degrees\n", rotation);
						break;
					case 's':
						i++;
						r = atoi(argv[i]);
						if (r >= 1 && r <= 150) { scaling = r; }
						                   else { scaling = 100; }
						L("scaling to %d\n", scaling);
						break;
					case 'R':
						i++;
						extractReverseHostPort(argv[i]);
						break;
					case 'v':
						i++;
						L("emteria.OS VNC server 1.1\n");
						exit(0);
				}
			}
			i++;
		}
	}

    initFlinger();
    L("Initializing VNC server:\n");
    L(" - width: %d\n", screenformat.width);
    L(" - height: %d\n", screenformat.height);
    L(" - bpp: %d\n", screenformat.bitsPerPixel);
    L(" - rgba: %d:%d:%d:%d\n", screenformat.redShift, screenformat.greenShift, screenformat.blueShift, screenformat.alphaShift);
    L(" - length: %d:%d:%d:%d\n", screenformat.redMax, screenformat.greenMax, screenformat.blueMax, screenformat.alphaMax);
    L(" - scaling: %d\n", scaling);
    L(" - port: %d\n", VNC_PORT);

	initInput(screenformat.width, screenformat.height);
    initVncServer(argc, argv);

    if (rhost)
    {
        L("Starting in reverse host mode\n");
        rfbClientPtr cl = rfbReverseConnection(vncscr, rhost, rport);
        if (cl == NULL) {
            L("Couldn't connect to remote host: %s\n",rhost);
        } else {
            cl->onHold = FALSE;
            rfbStartOnHoldClient(cl);
        }
    }

    while (TRUE)
    {
        long usec = (vncscr->deferUpdateTime + standby) * 1000;
        rfbProcessEvents(vncscr, usec);

        if (idle) { standby += 2; }
             else { standby = 2; }

        if (vncscr->clientHead == NULL)
        {
            idle = 1;
            standby = 50;
            continue;
        }

        // update screen if at least one client has requested
        for (rfbClientPtr client_ptr = vncscr->clientHead; client_ptr; client_ptr = client_ptr->next)
        {
            if (sraRgnEmpty(client_ptr->requestedRegion)) { continue; }

            update_screen();
            break;
        }
    }

    L("Cleaning up...\n");
    closeVncServer(0);
}
