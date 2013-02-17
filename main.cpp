/*
   hhugboy Game Boy emulator
   copyright 2013 taizou

   Based on GEST
   Copyright (C) 2003-2010 TM

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <stdio.h>

#include <string>
#include <iostream>

using namespace std;

#include <math.h>
#include <time.h>

#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

#include "strings.h"
#include "cheats.h"
#include "config.h"
#include "dialogs.h"

#include "main.h"
#include "debug.h"
#include "types.h"
#include "cpu.h"
#include "mainloop.h"
#include "sound.h"
#include "GB_gfx.h"
#include "SGB.h"
#include "devices.h"
#include "render.h"

#include "directdraw.h"
#include "directinput.h"


#include "menu.h"

#include "GB.h"

gb_system* GB = NULL;
gb_system* GB1 = NULL;
gb_system* GB2 = NULL;

const char* prg_version = "1.0";

// Windows stuff ----------------------------------------
HWND hwnd = NULL;           
HWND hwndCtrl = NULL;    

HINSTANCE hinst;

LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

wchar_t w_szClassName[] = L"GESTclass";
char emu_title[] = "hhugboy";
wchar_t w_emu_title[] = L"hhugboy";

char title_text[ROM_FILENAME_SIZE + 8];
wchar_t w_title_text[ROM_FILENAME_SIZE + 16];

int timer_id = 0;
int sizen_w,sizen_h; // Size needed for menu,borders,etc.



// Options ----------------------------------------------
bool paused = false;
bool menupause = false;

int control_pressed = 0; // control key pressed 

int current_controller = 0; // currently changing which controller? 

int speedup = 0;

program_configuration* options = NULL;

wstring gb1_loaded_file_name;

#ifdef ALLOW_DEBUG
bool debug = false;
#endif

#include "licnames.h"

int ramsize[9] = { 0, 2, 8, 32, 128, 64,64,64,8 }; // KBytes

int romsize(int number)
{
   switch(number)
   {
      case 0: return 32; 
      case 1: return 64; 
      case 2: return 128;  
      case 3: return 256; 
      case 4: return 512;   
      case 5: return 1024;  
      case 6: return 2048; 
      case 7: return 4096; 
      case 8: return 8192;
      case 0x52: return 1126; 
      case 0x53: return 1228; 
      case 0x54: return 1536;
      default: return 32;
   }
}

void PrintROMinfo()
{
   char text[500];
   //sprintf(text, "Rom name: \t%s\nGBC feature: \t%s\nNew Licensee: \t%X\nSGB feature: \t%s\nCardridge Type: \t%X\nROM Size: \t%d KBytes\nRAM Size: \t%d KBytes\nCountry: \t\t%X ,%s\nLicensee: \t%X - %s\nVersion: \t\t%X\nComplement check: %X - %s\nChecksum: \t%X", rom->name, rom->CGB==1?"yes":rom->CGB==2?"GBC only":"no", rom->newlic, rom->SGB?"yes":"no", rom->carttype,romsize(rom->ROMsize),ramsize[rom->RAMsize], rom->destcode, rom->destcode==0?"Japan":"non-Japan",rom->lic,lic_names[rom->lic],rom->version,rom->complement,rom->complementok?"(ok)":"(wrong)",rom->checksum);
   sprintf(text, str_table[ROM_INFO], GB1->rom->name, GB1->rom->CGB==1?str_table[STR_YES]:GB1->rom->CGB==2?str_table[GBC_ONLY]:str_table[STR_NO], GB1->rom->newlic, GB1->rom->SGB?str_table[STR_YES]:str_table[STR_NO], GB1->rom->carttype,romsize(GB1->rom->ROMsize),ramsize[GB1->rom->RAMsize], GB1->rom->destcode, GB1->rom->destcode==0?str_table[STR_JAPAN]:str_table[STR_NON_JAPAN],GB1->rom->lic,lic_names[GB1->rom->lic],GB1->rom->version,GB1->rom->complement,GB1->rom->complementok?str_table[CHECK_OK]:str_table[CHECK_WRONG],GB1->rom->checksum);
   debug_print(text);
}

void cleanup()
{
   if(GB1 != NULL) 
   {
      delete GB1;
      GB1 = NULL;
   }
   
   if(GB2 != NULL) 
   {
      delete GB2;
      GB2 = NULL;
   }

   if(options != NULL)
   {
       delete options;
       options = NULL;
   }
                  
   sgb_end();
   
   Kill_DD(); 
   
   Kill_DI();  
   
   FSOUND_Close();
}

int WINAPI WinMain(HINSTANCE hThisInstance,HINSTANCE hPrevInstance, LPSTR  lpszArgument,int nFunsterStil)
{
	
   WNDCLASSEX wincl;        
   MSG msg;    
   
   /* The Window structure */
   hinst = hThisInstance;
   wincl.hInstance = hThisInstance;
   wincl.lpszClassName = w_szClassName;
   wincl.lpfnWndProc = WindowProcedure;      
   wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
   wincl.cbSize = sizeof(WNDCLASSEX);

   wincl.hIcon = LoadIcon(hThisInstance, MAKEINTRESOURCE(ID_ICON));
   wincl.hIconSm = LoadIcon(hThisInstance, MAKEINTRESOURCE(ID_ICON));
   wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
   wincl.lpszMenuName = NULL;                 /* No menu */
   wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
   wincl.cbWndExtra = 0;                      /* structure or the window instance */
   wincl.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
   
   if(!RegisterClassEx(&wincl))
      return 0;
   
   emuMenu.init(hThisInstance);

   hwnd = CreateWindowEx(0,w_szClassName,w_emu_title,WS_SIZEBOX|WS_OVERLAPPEDWINDOW,150,150,2*160,2*144,HWND_DESKTOP,emuMenu.getMenu(),hThisInstance,NULL);
   
   RECT adjrect;
   GetClientRect(hwnd,&adjrect);
   
   sizen_w = 2*160-(adjrect.right-adjrect.left);
   sizen_h = 2*144-(adjrect.bottom-adjrect.top);

   options = new program_configuration;
   GB1 = new gb_system;

   if(!options || !GB1 || !GB1->init() || !sgb_init())
   { 
      debug_print(str_table[ERROR_MEMORY]); 
      PostMessage(hwnd, WM_QUIT, 0, 0L);  
   }

   GB = GB1;
   
   if(!Init_DD()) 
   { 
      debug_print(str_table[ERROR_DDRAW]); 
      PostMessage(hwnd, WM_QUIT, 0, 0L);  
   }
        
   LARGE_INTEGER cur_time;
   LARGE_INTEGER now_time;
   DWORD time_count = 16;
   LARGE_INTEGER perf_cnt;
   LARGE_INTEGER next_time;

   int perf_flag = 0;
   int time_to_sleep = 0;
     
   if(QueryPerformanceFrequency(&perf_cnt))
   {
      perf_flag = 1;
      time_count = (DWORD)(perf_cnt.QuadPart/59.7275);
      QueryPerformanceCounter(&next_time);
   } else 
   {
      next_time.QuadPart = timeGetTime();
      time_count = 33;
   }

   //Get program directory
   wchar_t buffer[PROGRAM_PATH_SIZE];
   GetModuleFileName(NULL,buffer,PROGRAM_PATH_SIZE);

	
   options->program_directory = buffer;
   size_t found = options->program_directory.rfind('\\'); // Cut program name
   if(found != string::npos)
   {
      options->program_directory = options->program_directory.substr(0,found);
   }

   if(!read_config_file())
      debug_print(str_table[ERROR_CFG_FILE_READ]);

   ShowWindow(hwnd, SW_SHOW);    

   if(!Init_DI())
   {
      debug_print(str_table[ERROR_DINPUT]);
      PostMessage(hwnd, WM_QUIT, 0, 0L);        
   }

   FSOUND_SetHWND(hwnd);
   if(FSOUND_Init(44100,2,0) == FALSE)
   {
       debug_print(str_table[ERROR_FMOD]);
       options->sound_on = -1;
   } else
      FSbuffer = FSOUND_Sample_Alloc(FSOUND_FREE,sound_buffer_total_len>>2,FSOUND_LOOP_NORMAL|FSOUND_16BITS|FSOUND_STEREO|FSOUND_SIGNED,44100,200,FSOUND_STEREOPAN,1);
   
   if(options->sound_on)
      channel_n = FSOUND_PlaySound(FSOUND_FREE,FSbuffer);
  
   DragAcceptFiles(hwnd,TRUE);

   int noArgs;
   LPWSTR *argList = CommandLineToArgvW(GetCommandLineW(), &noArgs);
   
   if(noArgs >= 2)
   {
      gb1_loaded_file_name = argList[1];
      GB1->load_rom(argList[1]); 

      if(GB1->romloaded)
      {
         GB1->reset();
              
         if(sgb_mode)
         {
         	setWinSize(256,224);
         } else
         {
         	setWinSize(160,144);
         }
                     
         if(!GB1->load_save())
            debug_print(str_table[ERROR_SAVE_FILE_READ]);
      

         swprintf(w_title_text,L"%s - %s",w_emu_title,GB1->rom_filename);
         SetWindowText(hwnd,w_title_text);
      }
   }

   for(;;)
   {
      if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) 
      {
         if(msg.message == WM_QUIT)
            break;
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      } else 
      if(!emulating || !GB1->romloaded || paused)
      {
         Sleep(50);
      } else
      {        
         #ifdef ALLOW_DEBUG                
         if(debug && !paused && romloaded[1])
         {
            GB->frames = 1;
            mainloop();
            GB->frames = 0;

            draw_debug_screen();
            continue;
         }
         #endif
         
         if(perf_flag)
            QueryPerformanceCounter(&cur_time);
         else
            cur_time.QuadPart = timeGetTime();

         //-----Reduce CPU usage----
         if(options->reduce_cpu_usage && !speedup && cur_time.QuadPart < next_time.QuadPart)
         {
            QueryPerformanceCounter(&cur_time);

            time_to_sleep = (int)floor(((next_time.QuadPart - cur_time.QuadPart)*1000) / perf_cnt.QuadPart); // Time to sleep (in milliseconds)
            if(--time_to_sleep < 0)
                time_to_sleep = 0;

            Sleep(time_to_sleep);

            do
            {
               //Sleep(0);

               QueryPerformanceCounter(&cur_time);
            } while(cur_time.QuadPart < next_time.QuadPart);
         }
         //-----Reduce CPU usage----

         if(speedup || cur_time.QuadPart >= next_time.QuadPart)
         {
             // Runs until frame
             if(!multiple_gb)
             {
               do
               {
                 GB1->mainloop();
               } while(GB1->frames < 1);
             } else
             {
               do
               {
                 GB = GB1;

                 GB1->mainloop();

                 GB = GB2;

                 GB2->mainloop();
               } while(GB1->frames < 1);
             }

             GB1->frames = 0;

             next_time.QuadPart = cur_time.QuadPart + time_count;

             //-----Auto frameskip----
             if(options->video_auto_frameskip)
             {
                QueryPerformanceCounter(&cur_time);
                
                if(cur_time.QuadPart > next_time.QuadPart)
                {
                   if(options->video_frameskip < 9)
                   {
                      ++options->video_frameskip;
                   }
                }
                else if(options->video_frameskip > 0)
                {
                   --options->video_frameskip;
                }
                
                if(multiple_gb)
                   swprintf(w_title_text,L"%s - %s --- %s, frameskip: %i",w_emu_title,GB1->rom_filename,GB2->rom_filename,options->video_frameskip);
                else
                   swprintf(w_title_text,L"%s - %s, frameskip: %i",w_emu_title,GB1->rom_filename,options->video_frameskip);
                SetWindowText(hwnd,w_title_text);                
             }
             //-----Auto frameskip----

             //-----Reduce CPU usage----
           //  if(options->reduce_cpu_usage && !speedup /*&& (!multiple_gb || multiple_gb && current_gb == 2)*/)
          /*   {
                QueryPerformanceCounter(&now_time);
                
                time_to_sleep = (int)floor(((next_time.QuadPart - now_time.QuadPart)*1000) / perf_cnt.QuadPart); // Time to sleep (in milliseconds)
                time_to_sleep -= 1; //Time to run sleep function+alpha
                                             
                if(time_to_sleep > 0)
                   Sleep(time_to_sleep);
             }*/
             //-----Reduce CPU usage----                   
          }  
      }
   }
   return msg.wParam;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)           
    {
        case WM_COMMAND:
            menuAction(wParam);
    	//char txt[100];
    	//sprintf(txt,"%d",wParam);
    	//debug_print(txt);

        break;
        case WM_INITMENU:
           FSOUND_SetMute(FSOUND_ALL,TRUE);
        break;
        case WM_EXITMENULOOP:
           if(GB1->romloaded && !paused)
              FSOUND_SetMute(FSOUND_ALL,FALSE);  
           if(sgb_mode || (options->GBC_SGB_border != OFF && border_uploaded))
              draw_border();
        break;
        case WM_ENTERSIZEMOVE:
           FSOUND_SetMute(FSOUND_ALL,TRUE); 
        break;
        case WM_EXITSIZEMOVE:
        {
           if(GB1->romloaded && !paused)   
              FSOUND_SetMute(FSOUND_ALL,FALSE); 

           POINT pt;
           GetClientRect(hwnd,&target_blt_rect);
           pt.x=pt.y=0;
           ClientToScreen(hwnd,&pt);
           OffsetRect(&target_blt_rect,pt.x,pt.y);

           if(multiple_gb)
           {
               int width = target_blt_rect.right - target_blt_rect.left;
               
               target_blt_rect.right = target_blt_rect.left + width / 2;
           }

           if(sgb_mode || (options->GBC_SGB_border != OFF && border_uploaded))
           {
              double width = ((double)(target_blt_rect.right-target_blt_rect.left)/256.0);
              double height = ((double)(target_blt_rect.bottom-target_blt_rect.top)/224.0);

              target_blt_rect.left += (long)round(48.0*width); 
              target_blt_rect.right = target_blt_rect.left + (long)round(160.0*width); 
              target_blt_rect.top += (long)round(40.0*height);
              target_blt_rect.bottom = target_blt_rect.top + (long)round(144.0*height);

              draw_border();
              if(sgb_mask == 1) draw_screen();
           }                     
           if(!GB1->romloaded || paused)
              draw_screen();           
        }
        break;
        case WM_SIZE:
        {
           POINT pt;
           GetClientRect(hwnd,&target_blt_rect);
           pt.x=pt.y=0;
           ClientToScreen(hwnd,&pt);
           OffsetRect(&target_blt_rect,pt.x,pt.y);

           if(multiple_gb)
           {
               int width = target_blt_rect.right - target_blt_rect.left;

               target_blt_rect.right = target_blt_rect.left + width / 2;
           }
           
           if(sgb_mode || (options->GBC_SGB_border != OFF && border_uploaded))
           {
              double width = ((double)(target_blt_rect.right-target_blt_rect.left)/256.0);
              double height = ((double)(target_blt_rect.bottom-target_blt_rect.top)/224.0);

              target_blt_rect.left += (long)round(48.0*width); 
              target_blt_rect.right = target_blt_rect.left + (long)round(160.0*width); 
              target_blt_rect.top += (long)round(40.0*height);
              target_blt_rect.bottom = target_blt_rect.top + (long)round(144.0*height);
              
              draw_border();
              if(sgb_mask == 1) draw_screen();
           }       
           if(!GB1->romloaded || paused)
              draw_screen();           
        }
        break;
        case WM_ACTIVATE:  
           if(GB1->romloaded && (sgb_mode || (options->GBC_SGB_border != OFF && border_uploaded)))
              draw_border();
           
           if(!GB1->romloaded || paused)
              draw_screen();
           
           control_pressed = 0;

           if((wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE))
           {
              if(!menupause)
              {
                 paused = false;
                 if(GB1->romloaded)
                    FSOUND_SetMute(FSOUND_ALL,FALSE);
              }      
           } else
           {
              paused = true;
              speedup = 0;
              FSOUND_SetMute(FSOUND_ALL,TRUE);
           }
        break;
        case WM_KEYDOWN:
            keyAction(wParam);
        break;
        case WM_KEYUP:
            if(wParam == VK_CONTROL)
               control_pressed = 0;
        break;
        case WM_DESTROY:            
            if(GB1->romloaded && !GB1->write_save())
               debug_print(str_table[ERROR_SAVE_FILE_WRITE]);

            if(GB2 && GB2->romloaded && !GB2->write_save())
               debug_print(str_table[ERROR_SAVE_FILE_WRITE]);            
            
            write_config_file();
            cleanup();
            PostQuitMessage(0);     
        break;
        case WM_DROPFILES:
        {
           bool romwasloaded = GB1->romloaded;
                  
           if(GB1->romloaded && !GB1->write_save())
              debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
           HDROP d_handle = (HDROP)wParam;      
           wchar_t drag_filename[500];
           DragQueryFileW(d_handle,0,drag_filename,500);
           if(!wcsstr(drag_filename,L".zip") && !wcsstr(drag_filename,L".ZIP") && !wcsstr(drag_filename,L".sgb") && !wcsstr(drag_filename,L".SGB") && !wcsstr(drag_filename,L".GB") && !wcsstr(drag_filename,L".gb"))
           {
              debug_print(str_table[NOT_A_ROM]);
              GB1->romloaded = false;
           } else {
		      gb1_loaded_file_name = drag_filename;
              GB1->load_rom(drag_filename);
           }
           
              
                  
           if(GB1->romloaded)
           {                    
              GB1->reset();
              
              int reswidth,resheight;
              
              if(sgb_mode)
              {
              	setWinSize(256,224);
              } else
              {
				setWinSize(160,144);
			  }
                     
              if(!GB1->load_save())
                 debug_print(str_table[ERROR_SAVE_FILE_READ]);
                  
              swprintf(w_title_text,L"%s - %s",w_emu_title,GB1->rom_filename);
              SetWindowText(hwnd,w_title_text);
              if(!paused)
                 FSOUND_SetMute(FSOUND_ALL,FALSE);   
           } else 
              if(romwasloaded && GB1->cartridge != NULL) 
                 GB1->romloaded = true;    
                 
           DragFinish(d_handle);  
           SetForegroundWindow(hwnd);  
        }
        break;
        default:                  
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

void menuAction(int menuOption)
{
    switch( menuOption ) {
            
         case IDM_RECENTROM0:
         case IDM_RECENTROM1:
         case IDM_RECENTROM2:
         case IDM_RECENTROM3:
         case IDM_RECENTROM4:
         case IDM_RECENTROM5:
         case IDM_RECENTROM6:
         case IDM_RECENTROM7:
         case IDM_RECENTROM8:
         case IDM_RECENTROM9:
         case IDM_FILEOPEN:
         {                  
              bool romwasloaded = GB1->romloaded;
              
              if(GB1->romloaded && !GB1->write_save())
                 debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
              
              if ( menuOption == IDM_FILEOPEN ) {
                 if(!DoFileOpen(hwnd,1))
                    debug_print(str_table[ERROR_ROM_LOAD]);
              } else { // its a Recent ROM 
                    if (options->recent_rom_names[menuOption - IDM_RECENTROM0] == L"")
                        break;
              
                    gb1_loaded_file_name = options->recent_rom_names[menuOption - IDM_RECENTROM0];
                    if (!GB1->load_rom(options->recent_rom_names[menuOption - IDM_RECENTROM0].c_str())) {
                        debug_print(str_table[ERROR_ROM_LOAD]);
                    }
              }
              

                                
              if(GB1->romloaded)
              {            
                 GB1->reset();
                 
                 if(!multiple_gb)
                 {
                    if(sgb_mode)
                    {
                        setWinSize(256,224);
        
                       draw_border();
                    } else
                    {
                       setWinSize(160,144);
                    }
                 }
        
                 if(!GB1->load_save())
                    debug_print(str_table[ERROR_SAVE_FILE_READ]);
        
                 if(multiple_gb)
                    swprintf(w_title_text,L"%s - %s --- %s",w_emu_title,GB1->rom_filename,GB2->rom_filename);
                 else
                    swprintf(w_title_text,L"%s - %s",w_emu_title,GB1->rom_filename);
        
                 SetWindowText(hwnd,w_title_text);
        
                 if(!paused)
                    FSOUND_SetMute(FSOUND_ALL,FALSE);   
              } else 
              if(romwasloaded && GB1->cartridge != NULL) 
              {
                 GB1->romloaded = true;
                 if(!paused)
                    FSOUND_SetMute(FSOUND_ALL,FALSE);                      
              }
         }               
         break;
        
         case IDM_FILEOPEN2:
         {
              if(!GB1->romloaded)
              {
                 debug_print("Load GB1 rom first.");
                 break;
              }
              init_gb2();
        
              resize_window(2*160,144);
                 
              POINT pt;
              GetClientRect(hwnd,&target_blt_rect);
              pt.x=pt.y=0;
              ClientToScreen(hwnd,&pt);
              OffsetRect(&target_blt_rect,pt.x,pt.y);
              target_blt_rect.right -= 160*options->video_size;
              
              bool romwasloaded = GB2->romloaded;
              
              if(GB2->romloaded && !GB2->write_save())
                 debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
        
              if(!DoFileOpen(hwnd,2)) 
                 debug_print(str_table[ERROR_ROM_LOAD]);
              
              if(GB2->romloaded)
              {        
                 GB2->reset();
        
                 if(!GB2->load_save())
                    debug_print(str_table[ERROR_SAVE_FILE_READ]);
                 
                 swprintf(w_title_text,L"%s - %s --- %s",w_emu_title,GB1->rom_filename,GB2->rom_filename);
                 SetWindowText(hwnd,w_title_text);
                 
                 if(!paused)
                    FSOUND_SetMute(FSOUND_ALL,FALSE);   
              } else 
              if(romwasloaded && GB2->cartridge != NULL) 
              {
                 GB2->romloaded = true;
                 
                 if(!paused)
                    FSOUND_SetMute(FSOUND_ALL,FALSE);                      
              }
         }            
         break;
        
         case IDM_FILEOPENSAMEASGB1:
             if(GB1->romloaded)
             {
                 init_gb2();
                 
                 resize_window(2*160,144);
        
                 POINT pt;
                 GetClientRect(hwnd,&target_blt_rect);
                 pt.x=pt.y=0;
                 ClientToScreen(hwnd,&pt);
                 OffsetRect(&target_blt_rect,pt.x,pt.y);
                 target_blt_rect.right-=160*options->video_size;
        
                 if(GB2->romloaded && !GB2->write_save())
                    debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
        
                 GB2->load_rom(gb1_loaded_file_name.c_str());
        
                 if(GB2->romloaded)
                 {
                    GB2->reset();
        
                    if(!GB2->load_save())
                       debug_print(str_table[ERROR_SAVE_FILE_READ]);
        
                    swprintf(w_title_text,L"%s - %s --- %s",w_emu_title,GB1->rom_filename,GB2->rom_filename);
                    SetWindowText(hwnd,w_title_text);
        
                    if(!paused)
                       FSOUND_SetMute(FSOUND_ALL,FALSE);
                 }
             }
         break;
              
         case IDM_FILECLOSE:
              FSOUND_SetMute(FSOUND_ALL,TRUE);
              SetWindowText(hwnd,w_emu_title);
              if(GB1->romloaded)
              {
                 if(!GB1->write_save())
                    debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
              }
              if(GB1->cartridge)
              {
                 delete [] GB1->cartridge;
                 GB1->cartridge = NULL;
              }
              GB1->romloaded = false;
        
              if(GB1->gfx_bit_count == 16)
              {
                 memset(GB1->gfx_buffer,0,sizeof(WORD)*160*144);
                 memset(GB1->gfx_buffer_old,0,sizeof(WORD)*160*144);
              } else
              {
                 memset(GB1->gfx_buffer,0,sizeof(DWORD)*160*144);
                 memset(GB1->gfx_buffer_old,0,sizeof(DWORD)*160*144);
              }
              draw_screen();                
         break;
        
         case IDM_LOADGB1SAVE:
             if(GB1 && GB1->romloaded && GB2 && GB2->romloaded)
             {
                GB2->reset();
                
                if(!GB2->load_save(true))
                  debug_print(str_table[ERROR_SAVE_FILE_READ]);
             }
         break;
        
         case IDM_FILECLOSE2:
              if(!GB2)
                 break;
              
              if(!GB1->romloaded) 
              {
                 FSOUND_SetMute(FSOUND_ALL,TRUE);
                 SetWindowText(hwnd,w_emu_title);
              }
              setWinSize(160,144);
        
              if(GB2->romloaded)
              {
                 if(!GB2->write_save())
                    debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
        
                 swprintf(w_title_text,L"%s - %s",w_emu_title,GB1->rom_filename);
                 SetWindowText(hwnd,w_title_text);
              }
              if(GB2->cartridge)
              {
                 delete [] GB2->cartridge;
                 GB2->cartridge = NULL;
              }
              GB2->romloaded = false;
              multiple_gb = 0;
              GB = GB1;
              
              POINT pt;
              GetClientRect(hwnd,&target_blt_rect);
              pt.x=pt.y=0;
              ClientToScreen(hwnd,&pt);
              OffsetRect(&target_blt_rect,pt.x,pt.y);       
         break;             
         
         case IDM_ROMINFO:
              if(GB1->romloaded)
                 PrintROMinfo();
         break;
                                  
         case IDM_FOLDERS:
            DialogBox(hinst, MAKEINTRESOURCE(ID_F_DIALOG), hwnd, (DLGPROC)FolderProc);
         break;     
         
         case IDM_CHEATS:
            DialogBox(hinst, MAKEINTRESOURCE(ID_CHT_DIALOG), hwnd, (DLGPROC)ChtProc);               
         break;
         case IDM_FILEEXIT:
              SendMessage(hwnd, WM_CLOSE, 0, 0L);
         break;
         
         case IDM_CPUSOFTRESET:
              soft_reset = 1;
         break;
        
         case IDM_CPUSOFTRESET2:
             if(GB2)
                 soft_reset = 2;
         break;
        
         case IDM_CPURESET:
              if(GB1->romloaded && !GB1->write_save())
                 debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
        
              GB1->reset();
        
              if(GB1->romloaded && !GB1->load_save())
                 debug_print(str_table[ERROR_SAVE_FILE_READ]);
         break;
         
         case IDM_CPURESET2:
              if(!GB2)
                  break;
              
              if(GB2->romloaded && !GB2->write_save())
                 debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
        
              GB2->reset();
        
              if(GB2->romloaded && !GB2->load_save())
                 debug_print(str_table[ERROR_SAVE_FILE_READ]);
         break;
                      
         case IDM_CPUPAUSE:
            if(menupause)
            {
               if(GB1->romloaded)
                  FSOUND_SetMute(FSOUND_ALL,FALSE);
               emuMenu.uncheckOption(IDM_CPUPAUSE);
            }
            else
            {
               FSOUND_SetMute(FSOUND_ALL,TRUE);
               emuMenu.checkOption(IDM_CPUPAUSE); 
            }
            menupause=!menupause;
            paused=!paused;
         break;
        
         case IDM_CPUSTATELOAD:
            if(GB1->romloaded)
               GB1->load_state();
         break;
        
         case IDM_CPU2STATELOAD:
            if(GB2 && GB2->romloaded)
               GB2->load_state();
         break;
        
         case IDM_CPUSTATESAVE:
            if(GB1->romloaded)	
               GB1->save_state();
         break;     
         
         case IDM_CPU2STATESAVE:
            if(GB2 && GB2->romloaded)
               GB2->save_state();
         break;
        
        // Switch state slot for GB1
        case IDM_CPUSTATESLOT0:
        case IDM_CPUSTATESLOT1:
        case IDM_CPUSTATESLOT2:
        case IDM_CPUSTATESLOT3:
        case IDM_CPUSTATESLOT4:
        case IDM_CPUSTATESLOT5:
        case IDM_CPUSTATESLOT6:
        case IDM_CPUSTATESLOT7:
        case IDM_CPUSTATESLOT8:
        case IDM_CPUSTATESLOT9:
            GB1_state_slot = menuOption - IDM_CPUSTATESLOT0;
            emuMenu.checkOption(menuOption);
        break;
    
        // Switch state slot for GB2
        case IDM_CPU2STATESLOT0:
        case IDM_CPU2STATESLOT1:
        case IDM_CPU2STATESLOT2:
        case IDM_CPU2STATESLOT3:
        case IDM_CPU2STATESLOT4:
        case IDM_CPU2STATESLOT5:
        case IDM_CPU2STATESLOT6:
        case IDM_CPU2STATESLOT7:
        case IDM_CPU2STATESLOT8:
        case IDM_CPU2STATESLOT9:
            GB2_state_slot = menuOption - IDM_CPU2STATESLOT0;
            emuMenu.checkOption(menuOption);
        break;
        
        // Switch GB type for GB1
        case IDM_CPUAUTO:           
        case IDM_CPUGBA:          
        case IDM_CPUGB:   
        case IDM_CPUGBP:          
        case IDM_CPUGBC:
        case IDM_CPUSGB:
        case IDM_CPUSGB2: {
            int newSysType = menuOption - IDM_GROUP_GB1; // should be equal to the appropriate SYS_ constant
            if(GB1->system_type != newSysType) {
                GB1->system_type = newSysType;    
                if(GB1->romloaded && !GB1->write_save())
                    debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
                GB1->reset();
                if(GB1->romloaded && !GB1->load_save())
                    debug_print(str_table[ERROR_SAVE_FILE_READ]);                
            }
            emuMenu.checkOption(menuOption);       
            break;       
        }
                      
        // Switch GB type for GB2
        case IDM_CPUAUTO2:          
        case IDM_CPUGBA2:
        case IDM_CPUGB2:         
        case IDM_CPUGBP2:
        case IDM_CPUGBC2: {
            if(!GB2) {
                init_gb2();
                multiple_gb = 0;
            }
            int newSysType = menuOption - IDM_GROUP_GB2; // should be equal to the appropriate SYS_ constant
            if(GB2->system_type != newSysType) {
                GB2->system_type = newSysType;     
                if(GB2->romloaded && !GB2->write_save())
                    debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
                GB2->reset();
                if(GB2->romloaded && !GB2->load_save())
                    debug_print(str_table[ERROR_SAVE_FILE_READ]);                
            }
            emuMenu.checkOption(menuOption);            
            break;            
        }
                                           
         case IDM_CPUGBCSGB:
            if(options->GBC_SGB_border == GBC_WITH_SGB_BORDER)
            {
               emuMenu.uncheckOption(IDM_CPUGBCSGB);
               options->GBC_SGB_border = OFF;
            } else
            {
               emuMenu.checkOption(IDM_CPUGBCSGB);
               options->GBC_SGB_border = GBC_WITH_SGB_BORDER;
            }            
         break;         
         case IDM_CPUGBCSGBI:
            if(options->GBC_SGB_border == GBC_WITH_INITIAL_SGB_BORDER)
            {
               emuMenu.uncheckOption(IDM_CPUGBCSGBI);
               options->GBC_SGB_border = OFF;
            } else
            {
               emuMenu.checkOption(IDM_CPUGBCSGBI);
               options->GBC_SGB_border = GBC_WITH_INITIAL_SGB_BORDER;
            }                    
         break;      
         
         case IDM_UNLAUTO:
         	options->unl_compat_mode = UNL_AUTO;
         	emuMenu.checkOption(IDM_UNLAUTO);
         break;
         case IDM_UNLNONE:
         	options->unl_compat_mode = UNL_NONE;
         	emuMenu.checkOption(IDM_UNLNONE);
         break;
         case IDM_UNLNIUTOUDE:
         	options->unl_compat_mode = UNL_NIUTOUDE;
         	emuMenu.checkOption(IDM_UNLNIUTOUDE);
         break;
         case IDM_UNLSINTAX:
         	options->unl_compat_mode = UNL_SINTAX;
         	emuMenu.checkOption(IDM_UNLSINTAX);
        break;
         
         case IDM_CPUSPFS9:
            if(options->speedup_skip_9frames)
            {
               emuMenu.uncheckOption(IDM_CPUSPFS9);
               options->speedup_skip_9frames = false;
            } else
            {
               emuMenu.checkOption(IDM_CPUSPFS9);
               options->speedup_skip_9frames = true;
            }                
         break;    
         case IDM_CPUSPSNDOFF:
            if(options->speedup_sound_off)
            {
               emuMenu.uncheckOption(IDM_CPUSPSNDOFF);
               options->speedup_sound_off = false;
            } else
            {
               emuMenu.checkOption(IDM_CPUSPSNDOFF);
               options->speedup_sound_off = true;
            }                
         break;       
         case IDM_CPUOPCODE:
            if(options->halt_on_unknown_opcode)
            {
               emuMenu.uncheckOption(IDM_CPUOPCODE);
               options->halt_on_unknown_opcode = false;
            } else
            {
               emuMenu.checkOption(IDM_CPUOPCODE);
               options->halt_on_unknown_opcode = true;
            }                
         break;       
         case IDM_OPTIONCPUUSAGE:
            if(options->reduce_cpu_usage)
            {
               emuMenu.uncheckOption(IDM_OPTIONCPUUSAGE);
               options->reduce_cpu_usage = false;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONCPUUSAGE);
               options->reduce_cpu_usage = true;
            }                
         break;      
         case IDM_OPTIONOPPOSITEDIRECTIONS:
            if(options->opposite_directions_allowed)
            {
               emuMenu.uncheckOption(IDM_OPTIONOPPOSITEDIRECTIONS);
               options->opposite_directions_allowed = false;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONOPPOSITEDIRECTIONS);
               options->opposite_directions_allowed = true;
            }                
         break;                            
         case IDM_DEVICENONE:
            connected_device = DEVICE_NONE;
            reset_devices();
        
            emuMenu.checkOption(IDM_DEVICENONE);                       
         break; 
         case IDM_DEVICEBARCODE:
            connected_device = DEVICE_BARCODE;
            reset_devices();
        
            emuMenu.checkOption(IDM_DEVICEBARCODE);                           
         break;                                                                                
         case IDM_OPTIONVIDEOSIZE1:
         {
              options->video_size = 1;
              if(!sgb_mode && !(options->GBC_SGB_border == GBC_WITH_INITIAL_SGB_BORDER && border_uploaded))
        		 setWinSize(160,144);
              else
              {
        		 setWinSize(256,224);
                 draw_border();
              }
        
              if(multiple_gb)
        		setWinSize(320,144);
         }
         break;
        
         case IDM_OPTIONVIDEOSIZE2:
         {            
              options->video_size = 2;
              if(!sgb_mode && !(options->GBC_SGB_border == GBC_WITH_INITIAL_SGB_BORDER && border_uploaded))
        		 setWinSize(160,144);
              else
              {
        		 setWinSize(256,224);
                 draw_border();
              }
        
              if(multiple_gb)
        		setWinSize(320,144);
         }
         break;
        
         case IDM_OPTIONVIDEOSIZE3:
         {       
              options->video_size = 3;
              if(!sgb_mode && !(options->GBC_SGB_border == GBC_WITH_INITIAL_SGB_BORDER && border_uploaded))
        		 setWinSize(160,144);
              else
              {
        		 setWinSize(256,224);
                 draw_border();
              }
        
              if(multiple_gb)
        		setWinSize(320,144);
                 
         }
         break;
        
         case IDM_OPTIONVIDEOSIZE4:
         {       
              options->video_size = 4;
              if(!sgb_mode && !(options->GBC_SGB_border == GBC_WITH_INITIAL_SGB_BORDER && border_uploaded))
        		 setWinSize(160,144);
              else
              {
        		 setWinSize(256,224);
                 draw_border();
              }
        
              if(multiple_gb)
        		setWinSize(320,144);
                 
         }
         break;
              
         case IDM_OPTIONVIDEOBW: 
            options->video_GB_color = BLACK_WHITE;
            emuMenu.checkOption(IDM_OPTIONVIDEOBW);    
        
            if(!GB->gbc_mode || !GB->rom->CGB)
            {
               memcpy(GB->GBC_BGP,GBC_DMGBG_palettes[0],sizeof(unsigned int)*4);
               memcpy(GB->GBC_OBP,GBC_DMGOBJ0_palettes[0],sizeof(unsigned int)*4);
               memcpy(GB->GBC_OBP+4,GBC_DMGOBJ1_palettes[0],sizeof(unsigned int)*4);
            }                   
         break;   
              
         case IDM_OPTIONVIDEOLCDBROWN:  
            options->video_GB_color = LCD_BROWN;
            emuMenu.checkOption(IDM_OPTIONVIDEOLCDBROWN);    
            
            if(!GB->gbc_mode || !GB->rom->CGB)
            {
               memcpy(GB->GBC_BGP,LCD_palette_brown,sizeof(unsigned int)*4);
               memcpy(GB->GBC_OBP,LCD_palette_brown,sizeof(unsigned int)*4);
               memcpy(GB->GBC_OBP+4,LCD_palette_brown,sizeof(unsigned int)*4);
            }                              
         break;    
         
         case IDM_OPTIONVIDEOLCDGREEN:  
            options->video_GB_color = LCD_GREEN;
            emuMenu.checkOption(IDM_OPTIONVIDEOLCDGREEN);                	
            
            if(!GB->gbc_mode || !GB->rom->CGB)
            {
               memcpy(GB->GBC_BGP,LCD_palette_green,sizeof(unsigned int)*4);
               memcpy(GB->GBC_OBP,LCD_palette_green,sizeof(unsigned int)*4);
               memcpy(GB->GBC_OBP+4,LCD_palette_green,sizeof(unsigned int)*4);
            }                              
         break;    
        
         case IDM_OPTIONVIDEOGRAY:  
            options->video_GB_color = GRAY;
            emuMenu.checkOption(IDM_OPTIONVIDEOGRAY);                	
            
            if(!GB->gbc_mode || !GB->rom->CGB)
            {
               memcpy(GB->GBC_BGP,GB_palette_gray,sizeof(unsigned int)*4);
               memcpy(GB->GBC_OBP,GB_palette_gray,sizeof(unsigned int)*4);
               memcpy(GB->GBC_OBP+4,GB_palette_gray,sizeof(unsigned int)*4);
            }                              
         break;    
                                   
         case IDM_VIDEOFILTERNONE:
            if(options->video_filter != VIDEO_FILTER_NONE)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERNONE);                                           
                                                 
               options->video_filter = VIDEO_FILTER_NONE;
               filter_width = 1;
               filter_height = 1;
               
               change_filter();
            }
         break;
         case IDM_VIDEOFILTERSOFT2X:
            if(options->video_filter != VIDEO_FILTER_SOFT2X)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERSOFT2X);                            
                                                 
               options->video_filter = VIDEO_FILTER_SOFT2X;
               filter_width=2;
               filter_height=2;
               
               change_filter();
            }
         break;     
         case IDM_VIDEOFILTERSOFTXX:
            if(options->video_filter != VIDEO_FILTER_SOFTXX)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERSOFTXX);                                      
                                                 
               options->video_filter = VIDEO_FILTER_SOFTXX;
               filter_width=8;
               filter_height=8;
               
               change_filter();
            }
         break;   
         case IDM_VIDEOFILTERSCALE2X:
            if(options->video_filter != VIDEO_FILTER_SCALE2X)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERSCALE2X);                                   
               
               options->video_filter = VIDEO_FILTER_SCALE2X;
               filter_width=2;
               filter_height=2;
               
               change_filter();
            }
         break;      
         case IDM_VIDEOFILTERSCALE3X:
            if(options->video_filter != VIDEO_FILTER_SCALE3X)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERSCALE3X);                                     
               
               options->video_filter = VIDEO_FILTER_SCALE3X;
               filter_width=3;
               filter_height=3;
               
               change_filter();
            }
         break;              
         case IDM_VIDEOFILTERBLUR:
            if(options->video_filter != VIDEO_FILTER_BLUR)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERBLUR);       
                              
               options->video_filter = VIDEO_FILTER_BLUR;
               filter_width=2;
               filter_height=2;
               
               change_filter();
            }
         break;                
         case IDM_VIDEOFILTERUSEMMX:
            if(options->video_filter_use_mmx)
            {
               emuMenu.uncheckOption(IDM_VIDEOFILTERUSEMMX);
               options->video_filter_use_mmx = false;
            } else
            {
               emuMenu.checkOption(IDM_VIDEOFILTERUSEMMX);
               options->video_filter_use_mmx = true;
            }
         break;          
        
         case IDM_VIDEOFILTERBORDERNONE:
            if(options->video_SGBborder_filter != VIDEO_FILTER_NONE)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERBORDERNONE);                                                     
                                                 
               options->video_SGBborder_filter = VIDEO_FILTER_NONE;
               border_filter_width=1;
               border_filter_height=1;
               
               change_filter();
            }
         break;
         case IDM_VIDEOFILTERBORDERSOFT2X:
            if(options->video_SGBborder_filter != VIDEO_FILTER_SOFT2X)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERBORDERSOFT2X);                                  
                                                 
               options->video_SGBborder_filter = VIDEO_FILTER_SOFT2X;
               border_filter_width=2;
               border_filter_height=2;
               
               change_filter();
            }
         break;     
         case IDM_VIDEOFILTERBORDERSOFTXX:
            if(options->video_SGBborder_filter != VIDEO_FILTER_SOFTXX)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERBORDERSOFTXX);                                
                                                 
               options->video_SGBborder_filter = VIDEO_FILTER_SOFTXX;
               border_filter_width=8;
               border_filter_height=8;
               
               change_filter();
            }
         break;   
         case IDM_VIDEOFILTERBORDERSCALE2X:
            if(options->video_SGBborder_filter != VIDEO_FILTER_SCALE2X)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERBORDERSCALE2X);                                   
               
               options->video_SGBborder_filter = VIDEO_FILTER_SCALE2X;
               border_filter_width=2;
               border_filter_height=2;
               
               change_filter();
            }
         break;     
         case IDM_VIDEOFILTERBORDERSCALE3X:
            if(options->video_SGBborder_filter != VIDEO_FILTER_SCALE3X)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERBORDERSCALE3X);                                   
               
               options->video_SGBborder_filter = VIDEO_FILTER_SCALE3X;
               border_filter_width = 3;
               border_filter_height = 3;
               
               change_filter();
            }
         break;                 
         case IDM_VIDEOFILTERBORDERBLUR:
            if(options->video_SGBborder_filter != VIDEO_FILTER_BLUR)
            {
               emuMenu.checkOption(IDM_VIDEOFILTERBORDERBLUR);   
                              
               options->video_SGBborder_filter = VIDEO_FILTER_BLUR;
               border_filter_width=2;
               border_filter_height=2;
               
               change_filter();
            }
         break;                
        
         case IDM_OPTIONVIDEOMIXOFF:
            emuMenu.checkOption(IDM_OPTIONVIDEOMIXOFF);
        
            options->video_mix_frames = MIX_FRAMES_OFF;
            if(dx_bitcount == 16)
               draw_screen = draw_screen16;
            else
               draw_screen = draw_screen32;
         break;
        
         case IDM_OPTIONVIDEOMIXON:
            emuMenu.checkOption(IDM_OPTIONVIDEOMIXON);
        
            options->video_mix_frames = MIX_FRAMES_ON;
            if(dx_bitcount == 16)
               draw_screen = draw_screen_mix16;
            else
               draw_screen = draw_screen_mix32;
         break;
        
         case IDM_OPTIONVIDEOMIXMORE:
            emuMenu.checkOption(IDM_OPTIONVIDEOMIXMORE);
        
            options->video_mix_frames = MIX_FRAMES_MORE;
            if(dx_bitcount == 16)
               draw_screen = draw_screen_mix16;
            else
               draw_screen = draw_screen_mix32;
         break;
         
         case IDM_OPTIONVIDEOFSAUTO:
            if(options->video_auto_frameskip)
            {
               options->video_auto_frameskip = false;
               emuMenu.uncheckOption(IDM_OPTIONVIDEOFSAUTO); 
            } else
            {
               options->video_auto_frameskip = true;
               emuMenu.checkOption(IDM_OPTIONVIDEOFSAUTO);
            }               
         break;             
         case IDM_OPTIONVIDEOFS0:
            options->video_frameskip = 0;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);             
         break;
         case IDM_OPTIONVIDEOFS1:
            options->video_frameskip = 1;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);                    
         break;             
         case IDM_OPTIONVIDEOFS2:
            options->video_frameskip = 2;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);                  
         break;                             
         case IDM_OPTIONVIDEOFS3:
            options->video_frameskip = 3;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);                  
         break;  
         case IDM_OPTIONVIDEOFS4:
            options->video_frameskip = 4;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);                      
         break;              
         case IDM_OPTIONVIDEOFS5:
            options->video_frameskip = 5;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);                      
         break;           
         case IDM_OPTIONVIDEOFS6:
            options->video_frameskip = 6;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);                     
         break;            
         case IDM_OPTIONVIDEOFS7:
            options->video_frameskip = 7;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);                    
         break;                     
         case IDM_OPTIONVIDEOFS8:
            options->video_frameskip = 8;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);                   
         break;                  
         case IDM_OPTIONVIDEOFS9:
            options->video_frameskip = 9;
            emuMenu.checkOption(IDM_OPTIONVIDEOFS0+options->video_frameskip);                        
         break;       
         case IDM_OPTIONVIDEOENBG:
            if(video_enable&VID_EN_BG)
            {
               emuMenu.uncheckOption(IDM_OPTIONVIDEOENBG);
               video_enable &= ~VID_EN_BG;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONVIDEOENBG);
               video_enable |= VID_EN_BG;                
            }              
         break;   
         case IDM_OPTIONVIDEOENWIN:
            if(video_enable&VID_EN_WIN)
            {
               emuMenu.uncheckOption(IDM_OPTIONVIDEOENWIN);
               video_enable &= ~VID_EN_WIN;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONVIDEOENWIN);
               video_enable |= VID_EN_WIN;                
            }              
         break;  
         case IDM_OPTIONVIDEOENSPRITE:
            if(video_enable&VID_EN_SPRITE)
            {
               emuMenu.uncheckOption(IDM_OPTIONVIDEOENSPRITE);
               video_enable &= ~VID_EN_SPRITE;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONVIDEOENSPRITE);
               video_enable |= VID_EN_SPRITE;                
            }              
         break;  
         case IDM_OPTIONVIDEOSPRLIM:
            if(options->video_sprite_limit)
            {
               emuMenu.uncheckOption(IDM_OPTIONVIDEOSPRLIM);
               options->video_sprite_limit = false;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONVIDEOSPRLIM);
               options->video_sprite_limit = true;
            }              
         break;          
         case IDM_OPTIONVIDEOLCDOFF:
            if(options->video_LCDoff_clear_screen)
            {
               emuMenu.uncheckOption(IDM_OPTIONVIDEOLCDOFF);
               options->video_LCDoff_clear_screen = false;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONVIDEOLCDOFF);
               options->video_LCDoff_clear_screen = true;
            }              
         break;    
         case IDM_OPTIONVIDEOGBCCOLORS:
            if(options->video_GBCBGA_real_colors)
            {
               emuMenu.uncheckOption(IDM_OPTIONVIDEOGBCCOLORS);
               options->video_GBCBGA_real_colors = false;
        
               mix_gbc_colors();
            } else
            {
               emuMenu.checkOption(IDM_OPTIONVIDEOGBCCOLORS);                
               options->video_GBCBGA_real_colors = true;
        
               if(GB1->romloaded && GB1->gbc_mode)
                  mix_gbc_colors();
            }
         break;
         case IDM_OPTIONVIDEOVISUALRUMBLE:
             if(options->video_visual_rumble)
             {
                emuMenu.uncheckOption(IDM_OPTIONVIDEOVISUALRUMBLE);
                options->video_visual_rumble = false;
             } else
             {
                emuMenu.checkOption(IDM_OPTIONVIDEOVISUALRUMBLE);
                options->video_visual_rumble = true;
             }
         break;
         case IDM_OPTIONCONTROL1:
            current_controller = 0;
            hwndCtrl = CreateDialog(hinst, MAKEINTRESOURCE(ID_CTRL_DIALOG), hwnd, (DLGPROC) CtrlProc); 
            ShowWindow(hwndCtrl, SW_SHOW); 
            SetFocus(hwndCtrl);
             
            Init_DI_change(hwndCtrl);
         break; 
         case IDM_OPTIONCONTROL2:
            current_controller = 1;
            hwndCtrl = CreateDialog(hinst, MAKEINTRESOURCE(ID_CTRLMULTI_DIALOG), hwnd, (DLGPROC) MultiCtrlProc); 
            ShowWindow(hwndCtrl, SW_SHOW); 
            SetFocus(hwndCtrl);
             
            Init_DI_change(hwndCtrl);
         break; 
         case IDM_OPTIONCONTROL3:
            current_controller = 2;
            hwndCtrl = CreateDialog(hinst, MAKEINTRESOURCE(ID_CTRLMULTI_DIALOG), hwnd, (DLGPROC) MultiCtrlProc); 
            ShowWindow(hwndCtrl, SW_SHOW); 
            SetFocus(hwndCtrl);
             
            Init_DI_change(hwndCtrl);
         break; 
         case IDM_OPTIONCONTROL4:
            current_controller = 3;
            hwndCtrl = CreateDialog(hinst, MAKEINTRESOURCE(ID_CTRLMULTI_DIALOG), hwnd, (DLGPROC) MultiCtrlProc); 
            ShowWindow(hwndCtrl, SW_SHOW); 
            SetFocus(hwndCtrl);
             
            Init_DI_change(hwndCtrl);
         break;                                        
         case IDM_OPTIONSENSOR:
            hwndCtrl = CreateDialog(hinst, MAKEINTRESOURCE(ID_SENSOR_DIALOG), hwnd, (DLGPROC) SensorProc); 
            ShowWindow(hwndCtrl, SW_SHOW); 
            SetFocus(hwndCtrl);
            
            Init_DI_change(hwndCtrl);
         break;        
         case IDM_OPTIONCONTROLJOYCH:
            hwndCtrl = CreateDialog(hinst, MAKEINTRESOURCE(ID_CTRLJOY_DIALOG), hwnd, (DLGPROC) JoyCtrlProc); 
            ShowWindow(hwndCtrl, SW_SHOW); 
            SetFocus(hwndCtrl);
            
            Init_DI_change_joy(hwndCtrl);
         break;                       
         case IDM_OPTIONCONTROLJOY1:
            if(options->use_joystick_input == 0)
            {
               options->use_joystick_input = -1;
               emuMenu.uncheckOption(IDM_OPTIONCONTROLJOY1);
            } else
            {
               options->use_joystick_input = 0;
               emuMenu.checkOption(IDM_OPTIONCONTROLJOY1);
            }
         break;    
         case IDM_OPTIONCONTROLJOY2:
            if(options->use_joystick_input == 1)
            {
               options->use_joystick_input = -1;
               emuMenu.uncheckOption(IDM_OPTIONCONTROLJOY2);
            } else
            {
               options->use_joystick_input = 1;
               emuMenu.checkOption(IDM_OPTIONCONTROLJOY2);       
            }
         break;  
         case IDM_OPTIONCONTROLJOY3:
            if(options->use_joystick_input == 2)
            {
               options->use_joystick_input = -1;
               emuMenu.uncheckOption(IDM_OPTIONCONTROLJOY3);
            } else
            {
               options->use_joystick_input = 2;
               emuMenu.checkOption(IDM_OPTIONCONTROLJOY3);
            }
         break; 
         case IDM_OPTIONCONTROLJOY4:
            if(options->use_joystick_input == 3)
            {
               options->use_joystick_input = -1;
               emuMenu.uncheckOption(IDM_OPTIONCONTROLJOY4);
            } else
            {
               options->use_joystick_input = 3;
               emuMenu.checkOption(IDM_OPTIONCONTROLJOY4);
            }
         break;
        
         case IDM_OPTIONCONTROL_AUTOFIRE_SPEED_FASTEST:
             emuMenu.checkOption(IDM_OPTIONCONTROL_AUTOFIRE_SPEED_FASTEST);
             options->autofire_speed = AUTOFIRE_DELAY_FASTEST;
         break;
        
         case IDM_OPTIONCONTROL_AUTOFIRE_SPEED_FAST:
             emuMenu.checkOption(IDM_OPTIONCONTROL_AUTOFIRE_SPEED_FAST);
             options->autofire_speed = AUTOFIRE_DELAY_FAST;
         break;
        
         case IDM_OPTIONCONTROL_AUTOFIRE_SPEED_MEDIUM:
             emuMenu.checkOption(IDM_OPTIONCONTROL_AUTOFIRE_SPEED_MEDIUM);
             options->autofire_speed = AUTOFIRE_DELAY_MEDIUM;
         break;
        
         case IDM_OPTIONCONTROL_AUTOFIRE_SPEED_SLOW:
             emuMenu.checkOption(IDM_OPTIONCONTROL_AUTOFIRE_SPEED_SLOW);
             options->autofire_speed = AUTOFIRE_DELAY_SLOW;
         break;
        
         case IDM_OPTIONSOUND:
              if(options->sound_on == -1)
                 break;
              if(options->sound_on)
              {
                 FSOUND_StopSound(channel_n);
                 emuMenu.uncheckOption(IDM_OPTIONSOUND);
                 options->sound_on = 0;
              }
              else
              {
                 channel_n = FSOUND_PlaySound(FSOUND_FREE,FSbuffer);
                 emuMenu.checkOption(IDM_OPTIONSOUND);
                 options->sound_on = 1;
              }
         break;           
         case IDM_OPTIONSOUNDENCH1:
            if(sound_enable&SND_EN_CH1)
            {
               emuMenu.uncheckOption(IDM_OPTIONSOUNDENCH1);
               sound_enable &= ~SND_EN_CH1;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONSOUNDENCH1);
               sound_enable |= SND_EN_CH1;                
            }              
         break;   
         case IDM_OPTIONSOUNDENCH2:
            if(sound_enable&SND_EN_CH2)
            {
               emuMenu.uncheckOption(IDM_OPTIONSOUNDENCH2);
               sound_enable &= ~SND_EN_CH2;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONSOUNDENCH2);
               sound_enable |= SND_EN_CH2;                
            }              
         break;  
         case IDM_OPTIONSOUNDENCH4:
            if(sound_enable&SND_EN_CH4)
            {
               emuMenu.uncheckOption(IDM_OPTIONSOUNDENCH4);
               sound_enable &= ~SND_EN_CH4;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONSOUNDENCH4);
               sound_enable |= SND_EN_CH4;                
            }              
         break;               
         case IDM_OPTIONSOUNDENCH3:
            if(sound_enable&SND_EN_CH3)
            {
               emuMenu.uncheckOption(IDM_OPTIONSOUNDENCH3);
               sound_enable &= ~SND_EN_CH3;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONSOUNDENCH3);
               sound_enable |= SND_EN_CH3;                
            }              
         break;   
         case IDM_OPTIONSOUNDLOWPASSNONE:
            options->sound_lowpass_filter = 0;
            emuMenu.checkOption(IDM_OPTIONSOUNDLOWPASSNONE);                                   
         break;                     
         case IDM_OPTIONSOUNDLOWPASS1:
            options->sound_lowpass_filter = LOWPASS_LEVEL1;
            emuMenu.checkOption(IDM_OPTIONSOUNDLOWPASS1);                                                           
         break;   
         case IDM_OPTIONSOUNDLOWPASS2:
            options->sound_lowpass_filter = LOWPASS_LEVEL2;
            emuMenu.checkOption(IDM_OPTIONSOUNDLOWPASS2);                                                             
         break;         
         case IDM_OPTIONSOUNDRSTEREO:
            options->sound_reverse_stereo = !options->sound_reverse_stereo;
            if (options->sound_reverse_stereo)
                emuMenu.checkOption(IDM_OPTIONSOUNDRSTEREO);
            else
                emuMenu.uncheckOption(IDM_OPTIONSOUNDRSTEREO);
         break;      
         case IDM_OPTIONSOUNDVOL1:
            options->sound_volume = VOLUME_1X;
            emuMenu.checkOption(IDM_OPTIONSOUNDVOL1);       
         break;         
         case IDM_OPTIONSOUNDVOL2:
            options->sound_volume = VOLUME_2X;
            emuMenu.checkOption(IDM_OPTIONSOUNDVOL2);                 
         break;       
         case IDM_OPTIONSOUNDVOL3:
            options->sound_volume = VOLUME_3X;
            emuMenu.checkOption(IDM_OPTIONSOUNDVOL3);                                                                             
         break;       
         case IDM_OPTIONSOUNDVOL4:
            options->sound_volume = VOLUME_4X;
            emuMenu.checkOption(IDM_OPTIONSOUNDVOL4);                                                                     
         break;                                                                                                                                              
         case IDM_HELPABOUT:
              char about_str[200];
              sprintf(about_str,str_table[ABOUT_STR],prg_version);
              MessageBoxA(NULL, about_str, emu_title, 0);
         break;
        
        }
}

void keyAction(int key)
{
    switch(key)
    { 
        case VK_CONTROL:
           control_pressed = 1;            
        break;
        case VK_ESCAPE: // QUIT
           SendMessage(hwnd, WM_CLOSE, 0, 0L);
        break;
        case VK_F2:
           if(GB1->romloaded)
              GB1->save_state();
        break;
        case VK_F3:

           if(++GB1_state_slot > 9)
              GB1_state_slot = 0;
           sprintf(dx_message,"%s %d",str_table[STATE_SLOT],GB1_state_slot);
           message_time = 60;
           message_GB = GB1;
           
           emuMenu.checkOption(IDM_CPUSTATESLOT0 + GB1_state_slot);
        break;
        case VK_F4:
           if(GB1->romloaded)
              GB1->load_state();
        break;
        case VK_F5:
            if(video_enable&VID_EN_BG)
            {
               emuMenu.uncheckOption(IDM_OPTIONVIDEOENBG);
               video_enable &= ~VID_EN_BG;
               sprintf(dx_message,"%s","BG off");
               message_time = 40;
               message_GB = GB1;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONVIDEOENBG);
               video_enable |= VID_EN_BG;                
               sprintf(dx_message,"%s","BG on");
               message_time = 40;
               message_GB = GB1;
            }              
        break;
        case VK_F6:
            if(video_enable&VID_EN_WIN)
            {
               emuMenu.uncheckOption(IDM_OPTIONVIDEOENWIN);
               video_enable &= ~VID_EN_WIN;
               sprintf(dx_message,"%s","WIN off");
               message_time = 40;
               message_GB = GB1;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONVIDEOENWIN);
               video_enable |= VID_EN_WIN;                
               sprintf(dx_message,"%s","WIN on");
               message_time = 40;
               message_GB = GB1;
            }              
        break;
        case VK_F7:
            if(video_enable&VID_EN_SPRITE)
            {
               emuMenu.uncheckOption(IDM_OPTIONVIDEOENSPRITE);
               video_enable &= ~VID_EN_SPRITE;
               sprintf(dx_message,"%s","Sprites off");
               message_time = 40;
               message_GB = GB1;
            } else
            {
               emuMenu.checkOption(IDM_OPTIONVIDEOENSPRITE);
               video_enable |= VID_EN_SPRITE;                
               sprintf(dx_message,"%s","Sprites on");
               message_time = 40;
               message_GB = GB1;
            }              
        break;
        case VK_F12: {
            
			gb_system* screenshotGb = GB1;
			if (control_pressed && GB2) {
			    screenshotGb = GB2;
			}
			
			if (!screenshotGb->romloaded) {
			    break;
			}
        	
        	wchar_t img_filename[275];
			wcscpy(img_filename,screenshotGb->rom_filename);  // get the unicode filename
			char img_filename_mb[275]; 

			size_t res = wcstombs(img_filename_mb,img_filename,275); // but then oh well, look what happened
			// due to a limitation of png++ i cba to work around right now
		
			// now figure out the first available filename
			char final_filename[275];
			int fileno = 1;
			bool fileok = false;
			while ( fileok == false ) {
				char tmp_filename[275];

				if (res == -1) { // -1 = could not convert, so take the internal name instead
    			    sprintf(tmp_filename,"[%s]_%04d.png",GB1->rom->name,fileno);
    			} else {
    			    sprintf(tmp_filename,"%s_%04d.png",img_filename_mb,fileno);
    			}

		        ifstream ifile(tmp_filename);
				if ((bool)ifile) {
					fileno++;
				} else {
					fileok = true;
					strcpy(final_filename,tmp_filename);
				}
			}
			
			// show a msg
            sprintf(dx_message,"%s","Screenshot");
            message_time = 40;
            message_GB = screenshotGb;

			// and then screenshot this thing
        	screenshotPng(final_filename,screenshotGb);
        	break;
        	
        }
        	
        #ifdef ALLOW_DEBUG
        case 'D': // DEBUG
           if(!control_pressed)
              break;
           debug = !debug;
        break;
        case 'F': // STEP
           if(GB->romloaded && paused && !debug)
           {
              frames = 0;
              mainloop();
           }
           if(debug && GB->romloaded)
           {
              frames = 1;
              mainloop();
              frames = 0;
              draw_debug_screen();
           }
        break;
        #endif
        case 'P': // PAUSE
           if(!control_pressed)
              break;
           
           if(menupause)
           {
              emuMenu.uncheckOption(IDM_CPUPAUSE);
              if(GB1->romloaded)
                 FSOUND_SetMute(FSOUND_ALL,FALSE);
           }
           else
           {
              FSOUND_SetMute(FSOUND_ALL,TRUE);
              emuMenu.checkOption(IDM_CPUPAUSE); 
           }
           menupause = !menupause;
           paused = !paused;
        break;
        case 'F': // RESET
           if(!control_pressed)
              break;

           soft_reset = 1;     
        break;            
        case 'R': // RESET
           if(!control_pressed)
              break;       
                   
           if(GB1->romloaded && !GB1->write_save())
              debug_print(str_table[ERROR_SAVE_FILE_WRITE]);
                 
           GB1->reset();
           
           if(GB1->romloaded && !GB1->load_save())
              debug_print(str_table[ERROR_SAVE_FILE_READ]);
        break;
        
        // change GBC palette for mono games
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '0':
        case 0xDC:
        case 0xBB: {
            int paletteNo;
            if(key >= '1' && key <= '9') {
                paletteNo = key - '1';
            } else if (key == '0') {
                paletteNo = 9;
            } else if (key == 0xDC) {
                paletteNo = 10;
            } else if (key == 0xBB) {
                paletteNo = 11;
            }
            if(GB1->gbc_mode && !GB1->rom->CGB) {
                memcpy(GB1->GBC_BGP,GBC_DMGBG_palettes[paletteNo],sizeof(unsigned int)*4);
                memcpy(GB1->GBC_OBP,GBC_DMGOBJ0_palettes[paletteNo],sizeof(unsigned int)*4);
                memcpy(GB1->GBC_OBP+4,GBC_DMGOBJ1_palettes[paletteNo],sizeof(unsigned int)*4);
            }
            if(GB2 && GB2->gbc_mode && !GB2->rom->CGB) {
                memcpy(GB2->GBC_BGP,GBC_DMGBG_palettes[paletteNo],sizeof(unsigned int)*4);
                memcpy(GB2->GBC_OBP,GBC_DMGOBJ0_palettes[paletteNo],sizeof(unsigned int)*4);
                memcpy(GB2->GBC_OBP+4,GBC_DMGOBJ1_palettes[paletteNo],sizeof(unsigned int)*4);
            }
            break; 
        }
                                                                                           
    }
}

void setWinSize(int width,int height) 
{
	// width, height are the base width and height for the screen size
	
   RECT winRect;
   GetWindowRect(hwnd,&winRect);
   MoveWindow(hwnd,winRect.left,winRect.top,options->video_size * width + sizen_w,options->video_size * height + sizen_h,TRUE);
   
   RECT adjrect;
   GetClientRect(hwnd,&adjrect);
   int sizen_w2 = options->video_size * width-(adjrect.right-adjrect.left);
   int sizen_h2 = options->video_size * height-(adjrect.bottom-adjrect.top);
   MoveWindow(hwnd,winRect.left,winRect.top,width*options->video_size+sizen_w+sizen_w2,height*options->video_size+sizen_h+sizen_h2,TRUE);
}
