#include <display_username.h>
#include "heltec.h"


void appendLen(char userName[])
{
    char dots[] = "...";
    int dotsLen = Heltec.display->getStringWidth(dots, strlen(dots));
    int userNameStrLen = strlen(userName);
    char shortenedUsername[32] = "\0";

    for(int i = 0; i < userNameStrLen; i++)
    {
        int userNameWidth = Heltec.display->getStringWidth(userName, strlen(shortenedUsername));

        if(userNameWidth + dotsLen >= 64)
        {
            shortenedUsername[i] = '\0';
            strcat(shortenedUsername, dots);
            memset(userName, '\0', strlen(userName));
            memcpy(userName, shortenedUsername, strlen(shortenedUsername));
            break;
        }
        shortenedUsername[i] = userName[i];
    }
}


void displayUsername(char *userName)
{
  Heltec.display->clear();
  Heltec.display->screenRotate(ANGLE_90_DEGREE);
  int str_len = Heltec.display->getStringWidth(userName, strlen(userName));


  if(str_len > 64)
  {
    appendLen(userName);
    str_len = Heltec.display->getStringWidth(userName, strlen(userName));
  }

  Heltec.display->drawString(32-(str_len/2), 0, userName);
  Heltec.display->display();
  Heltec.display->screenRotate(ANGLE_0_DEGREE);
  Heltec.display->clear();
}

void clearScreen(void) {

  Heltec.display->clear();
}