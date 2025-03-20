#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lv_conf.h"

#define MAX_ITEMS 4

int Hours;
int Mins;
int Secs;
int CDTime[16];
char CDTColor[16][8]={"R","A","G","G"};
int CDTimeInput[16];
char CDTColorTable[4][2] = {"X","G","A","R"};
char CommandTable[10][11] = {"XXX","FIX","VA","FoFL","ErFL","ATCS","MNL","PDET","MANO","XXX"};
char Command[10]="FIX";
char Mode[10]="FIXED";
char stageMode[10]="FIXED";
bool TimerSet=0;

int UART_TCPb=0;
int TCP_RECEIVED_TRA=0;

int stage=0;
int road;
int nextRoad;
int prevStage=0;
int decodeNEXT=0;
char TraInput[100];
char CurInput[100];
char NxtInput[100];
char NextCurInput[100];
char NextNxtInput[100];


char CurRoadColor='R';

char ATC1[311]="1,G20,A5";
char ATC2[311]="2,G20,A5";
char ATC3[311]="3,G20,A5";
char ATC4[311]="4,G20,A5";

char DEF1[311]="1,G20,A5";
char DEF2[311]="2,G20,A5";
char DEF3[311]="3,G20,A5";
char DEF4[311]="4,G20,A5";

char stage1[200];
char stage2[200];
char stage3[200];
char stage4[200];
char stage5[200];
char stage6[200];
char stage7[200];

char NEXT[100];

lv_obj_t *time_label;  // Global time label referenc
lv_obj_t *color_label[4];  // For 4 CDTColor labels
lv_obj_t *stripe_colors[4];  // For 4 CDTColor labels
lv_color_t colors[5];
static const char *TAG = "TrafficLight";

void init_colors() {
    colors[0] = lv_color_white();
    colors[1] = lv_color_make(0, 0, 255);
    colors[2] =  lv_color_make(0, 152, 255);
    colors[3] =  lv_color_make(0, 128, 0);
    colors[4] =  lv_color_black();
    
}




void DecodeTL(const char* input){
    char CurText;
    int CurValue;
    char payload[256]; 
        ESP_LOGI(TAG,"HOURS:%d,MINS:%d,SECS:%d",Hours,Mins,Secs);
        // sprintf(payload,"HOURS:%d,MINS:%d,SECS:%d",Hours,Mins,Secs); 
      
      ESP_LOGI(TAG,"DECODING Road: %d,Input:%s",road,input);
      sprintf(payload,"DECODING Road: %d,Input:%s",road,input);
    //   uart_write_string_ln(payload);
   
     if (sscanf(input, "%c%d", &CurText, &CurValue) == 2) {
        sprintf(CDTColor[road-1], "%c", CurText);  // Store character as string
        CDTime[road-1] = CurValue;
        CDTimeInput[road-1]=CurValue;

        sprintf(Command, "ATC");
        for(int i = 0; i < MAX_ITEMS; i++) {
            if(i != road-1) {
                sprintf(CDTColor[i], "R"); 
                CDTime[i] = -1;
                CDTimeInput[i]=0;
            }
        }
    } 
 
}

void DecodeStage(const char* input){
    ESP_LOGI(TAG,"waiting to start decoding.%s",input);
  
    sscanf(input, "%d,%[^,],%99[^:]",&road,CurInput,NextCurInput);
    DecodeTL(CurInput);
    CurRoadColor='G';
  

}



void decrement_CDTColor() {
    for (int i = 0; i < 4; i++) 
    {
        int value;

        if(CDTime[road-1]==0 && CurRoadColor == 'G')
        {
               
                   CurRoadColor = 'R';
                   DecodeTL(NextCurInput);
                
             //    else if(CurRoadColor=='R')
             //    {
             //       CurRoadColor = 'G';
             //     //   road = nextRoad;
             //       strcpy(CurInput,NxtInput);
             //       strcpy(NextCurInput,NextNxtInput);
             //       DecodeTL(CurInput);
             //    }            
         }
 
    
 
         else if(!UART_TCPb && CDTime[i] == 0 && stage < 4 ) {
             stage++;
             if(prevStage != stage && stage <= 4) {
                 switch(stage) {
                     case 1:
                         if(strcmp(stageMode, "FIXED") == 0) {
                             DecodeStage(DEF1);
                         } else {
                             DecodeStage(ATC1);
                         }
                         break;
                     case 2:
                         if(strcmp(stageMode, "FIXED") == 0) {
                             DecodeStage(DEF2);
                         } else {
                             DecodeStage(ATC2);
                         }
                         break;
                     case 3:
                         if(strcmp(stageMode, "FIXED") == 0) {
                             DecodeStage(DEF3);
                         } else {
                             DecodeStage(ATC3);
                         }
                         break;
                     case 4:
                         if(strcmp(stageMode, "FIXED") == 0) {
                             DecodeStage(DEF4);
                         } else {
                             DecodeStage(ATC4);
                         }
                         if(strcmp(Mode, "FIXED") != 0) {
                             strcpy(stageMode, "SERVER");
                         }
                         else{
                             strcpy(stageMode, "FIXED");
                         }
                         stage = 0;
                         break;
                 }
                 prevStage = stage;
             }
         }
        if(CDTimeInput[i]==0)
        {
            lv_label_set_text_fmt(color_label[i], "%s", Command);
        }
      
        if (color_label[i] != NULL && CDTimeInput[i]!=0) {
            lv_label_set_text_fmt(color_label[i], "%d", CDTime[i]);
           // ESP_LOGI(TAG,"Color Label %02d changed to %02d",i,CDTime[i]);
        }
        if (CDTColor[i][0] != '\0')  // ✅ Correct null character check
            {
                if (strstr(CDTColor[i], "R") != NULL)
                {
                    lv_obj_set_style_bg_color(stripe_colors[i], colors[1], LV_PART_MAIN);
                }
                else if (strstr(CDTColor[i], "A") != NULL)
                {
                    lv_obj_set_style_bg_color(stripe_colors[i], colors[2], LV_PART_MAIN);
                }
                else if (strstr(CDTColor[i], "G") != NULL)
                {
                    lv_obj_set_style_bg_color(stripe_colors[i], colors[3], LV_PART_MAIN);
                }
              
            }  
        if (CDTime[i] > 0)
            CDTime[i]--;   
    }
}







// ⏰ Timer callback function
static void update_time_label(lv_timer_t *timer) {
    Secs++;
    if (Secs >= 60) {
        Secs = 0;
        Mins++;
        if (Mins >= 60) {
            Mins = 0;
            Hours++;
            if (Hours >= 24) {
                Hours = 0;
            }
        }
    }
        //ESP_LOGI(TAG,"Time is - %02d:%02d:%02d",Hours,Mins,Secs);
        decrement_CDTColor(); 

    
        if (time_label != NULL) {
            lv_label_set_text_fmt(time_label, "%02d:%02d:%02d",Hours, Mins, Secs);
        }
       
      
}


// ✅ In your displayStripes() or app_main() function:



void displayLights(void) {
    init_colors();

    lv_obj_t * parent = lv_scr_act();
    lv_obj_set_style_bg_color(parent, lv_color_white(), LV_PART_MAIN); // Set white background
    static lv_style_t style_label;
   
    const char *color_names[] = {"WHITE", "RED", "ORANGE", "GREEN", "GREEN"};
    
    int y_offset = 0;
    int stripe_height = 50; // Adjust stripe height as needed

    for (int i = 0; i < 5; i++) {
        // Create a colored stripe
        lv_obj_t * stripe = lv_obj_create(parent);
        lv_obj_set_size(stripe, lv_pct(100), stripe_height);
        lv_obj_align(stripe, LV_ALIGN_TOP_LEFT, 0, y_offset);
     
        if(i==0)
        {
            time_label = lv_label_create(stripe);
            lv_label_set_text_fmt(time_label,"%d:%d:%d",Hours,Mins,Secs);
            lv_obj_set_style_text_color(time_label, lv_color_black(), LV_PART_MAIN);
            lv_obj_set_style_bg_color(stripe,colors[0], LV_PART_MAIN);
            lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0); // Center text within the stripe
            lv_style_init(&style_label);
            lv_style_set_text_font(&style_label, &lv_font_montserrat_22);
            lv_obj_add_style(time_label, &style_label, LV_PART_MAIN);

        }

        if(i>0)
        {
            stripe_colors[i-1]=stripe;
        if(strstr(CDTColor[i-1],"R")!=NULL)
        {
            lv_obj_set_style_bg_color(stripe_colors[i-1], colors[1], LV_PART_MAIN);
        }
        else if(strstr(CDTColor[i-1],"A")!=NULL)
        {
            lv_obj_set_style_bg_color(stripe_colors[i-1], colors[2], LV_PART_MAIN);
        }
        else if(strstr(CDTColor[i-1],"G")!=NULL)
        {
            lv_obj_set_style_bg_color(stripe_colors[i-1], colors[3], LV_PART_MAIN);
        }
        else{
            lv_obj_set_style_bg_color(stripe_colors[i-1], lv_color_black(), LV_PART_MAIN);
        }
        color_label[i - 1] = lv_label_create(stripe_colors[i-1]);  // Create label and assign to array
        lv_label_set_text_fmt(color_label[i - 1], "%d", CDTime[i - 1]);
        lv_obj_set_style_text_color(color_label[i - 1], lv_color_white(), LV_PART_MAIN);
        lv_obj_align(color_label[i - 1], LV_ALIGN_CENTER, 0, 0); // Center text within the stripe
        lv_style_init(&style_label);
        lv_style_set_text_font(&style_label, &lv_font_montserrat_22);
        lv_obj_add_style(color_label[i - 1], &style_label, LV_PART_MAIN);
        
        }
      
     
        // Create label for the color name
       
      
        // lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN); // Default to white text

    

      
        y_offset += stripe_height; // Move to next stripe position
    }
    if (TimerSet == 0)
    {
        lv_timer_create(update_time_label, 1000, NULL);  // 1000ms = 1s
        TimerSet = 1;        
    }
    
    // lv_timer_create(update_time_label, 1000, NULL);
}