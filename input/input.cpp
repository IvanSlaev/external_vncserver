/*
droid VNC server  - a vnc server for android
Copyright (C) 2011 Jose Pereira <onaips@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "common.h"
#include "log.h"
#include "suinput.h"

#include "input.h"

int inputfd = -1;
// keyboard code modified from remote input by http://www.math.bme.hu/~morap/RemoteInput/

// q,w,e,r,t,y,u,i,o,p,a,s,d,f,g,h,j,k,l,z,x,c,v,b,n,m
int qwerty[] = {30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};
// ,!,",#,$,%,&,',(,),*,+,,,-,.,/
int spec1[] = {57,2,40,4,5,6,8,40,10,11,9,13,51,12,52,52};
int spec1sh[] = {0,1,1,1,1,1,1,0,1,1,1,1,0,0,0,1};
// :,;,<,=,>,?,@
int spec2[] = {39,39,227,13,228,53,3};
int spec2sh[] = {1,0,1,1,1,1,1};
// [,\,],^,_,`
int spec3[] = {26,43,27,7,12,399};
int spec3sh[] = {0,0,0,1,1,0};
// {,|,},~
int spec4[] = {26,43,27,215,14};
int spec4sh[] = {1,1,1,1,0};

void initInput(int width, int height)
{
	L("Initializing keyboard and touch...\n");
	struct input_id id =
	{
		BUS_VIRTUAL, /* Bus type: 0x06*/
		1, /* Vendor id. */
		1, /* Product id. */
		1  /* Version id. */
	};

	if ((inputfd = suinput_open("VNC", &id, width, height)) == -1)
	{
		L("Cannot create virtual input devices\n");
	}
}

int keysym2scancode(rfbBool down, rfbKeySym c, int *sh, int *alt)
{
	int real = 1;

	if ('a' <= c && c <= 'z')
	{
		return qwerty[c - 'a'];
	}
	if ('A' <= c && c <= 'Z')
	{
		(*sh) = 1;
		return qwerty[c - 'A'];
	}

	if (c == '0')
		return 11;
	if ('1' <= c && c <= '9')
		return c-'1'+2;

	if (32 <= c && c <= 47)
	{
		(*sh) = spec1sh[c-32];
		return spec1[c-32];
	}
	if (58 <= c && c <= 64)
	{
		(*sh) = spec2sh[c-58];
		return spec2[c-58];
	}
	if (91 <= c && c <= 96)
	{
		(*sh) = spec3sh[c-91];
		return spec3[c-91];
	}
	if (123 <= c && c <= 127)
	{
		(*sh) = spec4sh[c-123]; 
		return spec4[c-123];
	}

	switch(c)
	{
		case 0xff08:
			return 14;// backspace
		case 0xff09: 
			return 15;// tab
		case 1: 
			(*alt)=1;
			return 34;// ctrl+a
		case 3: 
			(*alt)=1; 
			return 46;// ctrl+c
		case 4: 
			(*alt)=1; 
			return 32;// ctrl+d
		case 18:
			(*alt)=1; 
			return 31;// ctrl+r
		case 0xff0D: 
			return 28;// enter
		case 0xff1B: 
			return 158;// esc -> back
		case 0xFF51: 
			return 105;// left -> DPAD_LEFT
		case 0xFF53: 
			return 106;// right -> DPAD_RIGHT
		case 0xFF54: 
			return 108;// down -> DPAD_DOWN
		case 0xFF52:
			return 103;// up -> DPAD_UP
		//case 360:
		//	return 232;// end -> DPAD_CENTER (ball click)
		case 0xff50:
			return KEY_HOME;// home 
		case 0xFFC8:
			return 0; //F11 ?
		case 0xFFC9:
			return 0; // F10 or F12?
		case 0xffc1:
		//	down ? rotate(-1) : 0;
			return 0; // F4 rotate
		case 0xffff: 
			return 158;// del -> back
		case 0xff55: 
			return 229;// PgUp -> menu
		case 0xffcf: 
			return 127;// F2 -> search
		case 0xffe3: 
			return 127;// left ctrl -> search
		case 0xff56: 
			return 61;// PgUp -> call
		case 0xff57: 
			return 107;// End -> endcall
		case 0xffc2: 
			return 211;// F5 -> focus
		case 0xffc3: 
			return 212;// F6 -> camera
		case 0xffc4: 
			return 150;// F7 -> explorer
		case 0xffc5: 
			return 155;// F8 -> envelope

		case 50081:
		case 225:
			(*alt)=1;
			if (real)
				return 48; //a with acute
			return 30; //a with acute -> a with ring above
			
		case 50049:
		case 193:
			(*sh)=1;
			(*alt)=1;
			if (real)
				return 48; //A with acute 
			return 30; //A with acute -> a with ring above
			
		case 50089:
		case 233: 
			(*alt)=1;
			return 18; //e with acute
			
		case 50057:
		case 201:
			(*sh)=1;
			(*alt)=1;
			return 18; //E with acute
			
		case 50093:
		case 0xffbf:
			(*alt)=1; 
			if (real)
				return 36; //i with acute 
			return 23; //i with acute -> i with grave
			
		case 50061:
		case 205: 
			(*sh)=1;
			(*alt)=1; 
			if (real)
				return 36; //I with acute 
			return 23; //I with acute -> i with grave
			
		case 50099: 
		case 243:
			(*alt)=1; 
			if (real) 
				return 16; //o with acute 
			return 24; //o with acute -> o with grave
			
		case 50067: 
		case 211:
			(*sh)=1;
			(*alt)=1; 
			if (real) 
				return 16; //O with acute 
			return 24; //O with acute -> o with grave
			
		case 50102:
		case 246: 
			(*alt)=1;
			return 25; //o with diaeresis
			
		case 50070:
		case 214:
			(*sh)=1;
			(*alt)=1;
			return 25; //O with diaeresis
		
		case 50577: 
		case 245:
			(*alt)=1; 
			if (real)
				return 19; //Hungarian o 
			return 25; //Hungarian o -> o with diaeresis

		case 50576:
		case 213: 
			(*sh)=1;
			(*alt)=1;
			if (real)
				return 19; //Hungarian O
			return 25; //Hungarian O -> O with diaeresis

		case 50106:
		//case 0xffbe:
		//	(*alt)=1;
		// 	if (real)
		//		return 17; //u with acute
		// 	return 22; //u with acute -> u with grave
		case 50074:
		case 218: 
			(*sh)=1;
			(*alt)=1; 
			if (real) 
				return 17; //U with acute 
			return 22; //U with acute -> u with grave
		case 50108:
		case 252: 
			(*alt)=1;
			return 47; //u with diaeresis
			
		case 50076:
		case 220:
			(*sh)=1;
			(*alt)=1;
			return 47; //U with diaeresis
			
		case 50609:
		case 251: 
			(*alt)=1; 
			if (real) 
				return 45; //Hungarian u 
			return 47; //Hungarian u -> u with diaeresis
			
		case 50608:
		case 219: 
			(*sh)=1;
			(*alt)=1;
			if (real)
				return 45; //Hungarian U 
			return 47; //Hungarian U -> U with diaeresis

		default:
			return 0;
	}
}

void keyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
	//L("Got key: %04x (down=%d)\n", (unsigned int)key, (int)down);
	setIdle(0);

	int shift = 0;
	int alt = 0;
	int code = 0;

	if (inputfd == -1)
		return;

	if ((code = keysym2scancode(down, key, &shift, &alt)))
	{
		if (key && down)
		{
			if (shift) suinput_press(inputfd, 42); //left shift
			if (alt) suinput_press(inputfd, 56); //left alt
			suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);

			suinput_press(inputfd, code);
			suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);

			suinput_release(inputfd, code);
			suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);

			if (alt) suinput_release(inputfd, 56); //left alt
			if (shift) suinput_release(inputfd, 42); //left shift
			suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
		}
	}
}

void ptrEvent(int buttonMask, int x, int y, rfbClientPtr cl)
{
	static int leftClicked = 0;
	static int middleClicked = 0;
	static int rightClicked = 0;
	static int scrollUp = 0;
	static int scrollDown = 0;

	//L("Process event (%d, %d) with mask %u\n", x, y, buttonMask);
	if (inputfd == -1)
		return;

	//transformTouchCoordinates(&x,&y,cl->screen->width,cl->screen->height);
	setIdle(0);

	if ((buttonMask & 1) && leftClicked) // left btn pressed and moving
	{
		static int i=0;
		i = i+1;

		// some tweak to not report every move event
		if (i % 10 == 1)
		{
			suinput_write(inputfd, EV_ABS, ABS_X, x);
			suinput_write(inputfd, EV_ABS, ABS_Y, y);
			suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
		}
	}
	else if (buttonMask & 1) // left btn pressed
	{
		leftClicked = 1;

		suinput_write(inputfd, EV_ABS, ABS_X, x);
		suinput_write(inputfd, EV_ABS, ABS_Y, y);
		suinput_write(inputfd, EV_KEY, BTN_TOUCH, 1);
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
	}
	else if (leftClicked) // left btn released
	{
		leftClicked=0;
		suinput_write(inputfd, EV_ABS, ABS_X, x);
		suinput_write(inputfd, EV_ABS, ABS_Y, y);
		suinput_write(inputfd, EV_KEY, BTN_TOUCH, 0);
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
	}

	if (buttonMask & 2) // mid btn pressed
	{
		middleClicked=1;
		suinput_press(inputfd, KEY_END);
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
	}
	else if (middleClicked) // mid btn released
	{
		middleClicked=0;
		suinput_release(inputfd, KEY_END);
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
	}

	if (buttonMask & 4) // right btn pressed
	{
		rightClicked=1;
		suinput_press(inputfd, 158); // back key
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
	}
	else if (rightClicked) // right button released
	{
		rightClicked=0;
		suinput_release(inputfd, 158);
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
	}

	if (buttonMask & 8) // scroll up started
	{
		scrollUp=1;
	}
	else if (scrollUp) // scroll up finished
	{
		scrollUp=0;
		suinput_write(inputfd, EV_REL, REL_X, -(cl->screen->width * 2));
		suinput_write(inputfd, EV_REL, REL_Y, -(cl->screen->height * 2));
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
		suinput_write(inputfd, EV_REL, REL_X, x);
		suinput_write(inputfd, EV_REL, REL_Y, y);
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
		suinput_write(inputfd, EV_REL, REL_WHEEL, 1);
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
	}

	if (buttonMask & 16) // scroll down started
	{
		scrollDown=1;
	}
	else if (scrollDown) // scroll down finished
	{
		scrollDown=0;
		suinput_write(inputfd, EV_REL, REL_X, -(cl->screen->width * 2));
		suinput_write(inputfd, EV_REL, REL_Y, -(cl->screen->height * 2));
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
		suinput_write(inputfd, EV_REL, REL_X, x);
		suinput_write(inputfd, EV_REL, REL_Y, y);
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
		suinput_write(inputfd, EV_REL, REL_WHEEL, -1);
		suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
	}
}

inline void transformTouchCoordinates(int *x, int *y, int width, int height)
{
	int old_x = *x;
	int old_y = *y;
	int rotation = getCurrentRotation();

	if (rotation == 0)
	{
		*x = old_x / width;
		*y = old_y / height;
	}
	else if (rotation == 90)
	{
		*x = old_y / height;
		*y = (width - old_x) / width;
	}
	else if (rotation == 180)
	{
		*x = (width - old_x) / width;
		*y = (height - old_y) / height;
	}
	else if (rotation == 270)
	{
		*y = old_x / width;
		*x = (height - old_y) * height;
	}

	L("Transformed click coordinates: (%d, %d) -> (%d, %d)\n", old_x, old_y, *x, *y);
}

void cleanupInput()
{
	if (inputfd != -1)
	{
		suinput_close(inputfd);
	}
}
