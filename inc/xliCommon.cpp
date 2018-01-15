//  xliCommon.cpp - Xlight global variables & functions
#include "xliCommon.h"

//--------------------------------------------------
// Tools & Helpers
//--------------------------------------------------
int wal_stricmp(const char *a, const char *b) {
  uint8_t ca, cb;
  do {
     ca = (unsigned char) *a++;
     cb = (unsigned char) *b++;
     ca = toupper(ca);
     cb = toupper(cb);
   } while (ca == cb && ca != '\0');
   return (int)(ca - cb);
}

int wal_strnicmp(const char *a, const char *b, uint8_t len) {
  uint8_t ca, cb,	i = 0;
  do {
     ca = (unsigned char) *a++;
     cb = (unsigned char) *b++;
     ca = toupper(ca);
     cb = toupper(cb);
   } while (ca == cb && ca != '\0' && ++i < len );
   return (int)(ca - cb);
}

uint8_t h2i(const char c)
{
	uint8_t i = 0;
	if (c <= '9')
		i += c - '0';
	else if (c >= 'a')
		i += c - 'a' + 10;
	else
		i += c - 'A' + 10;
	return i;
}

char* PrintUint64(char *buf, uint64_t value, bool bHex) {
	if (buf != NULL) {
		if (value > 0xFFFFFFFFLL) {
			if (bHex) {
				uint32_t part1 = value >> 32;
				uint32_t part2 = value & 0xFFFFFFFFLL;
				sprintf(buf, "0x%X%04X", part1, part2);
			}
			else {
				uint8_t digits[20];
				uint64_t tmpV = value;
				int len = 0;
				while (tmpV > 0x00LL) {
					digits[len++] = tmpV % 10;
					tmpV /= 10;
				}
				for (int i = len - 1; i >= 0; i--) {
					buf[len - 1 - i] = digits[i] + '0';
				}
				buf[len] = NULL;
			}
		}
		else {
			if (bHex) {
				sprintf(buf, "0x%04X", value);
			}
			else {
				sprintf(buf, "%d", value);
			}
		}
	}
	return buf;
}

char* PrintMacAddress(char *buf, const uint8_t *mac, char delim, bool bShort)
{
	if (buf != NULL) {
		if( bShort ) {
			// 6 sections
			sprintf(buf, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X",
					mac[0], delim, mac[1], delim, mac[2], delim, mac[3], delim, mac[4], delim, mac[5]);
		} else {
			// 8 sections
			sprintf(buf, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X%c%02X%c%02X",
					mac[0], delim, mac[1], delim, mac[2], delim, mac[3], delim, mac[4], delim, mac[5],
					delim, mac[7], delim, mac[7]);
		}
  }

  return buf;
}

uint64_t StringToUInt64(const char *strData)
{
  // Hex or decimal
  int i, nLen;
  bool bHex = false;
  nLen = strlen(strData);
  for( i=0; i< nLen; i++ ) {
    if( (strData[i] >= 'a' && strData[i] <= 'f') ||
        (strData[i] >= 'A' && strData[i] <= 'F') ) {
          bHex = true;
    } else if( strData[i] >= '0' && strData[i] <= '9' ) {
      continue;
    } else {
      // Invalid string
      return 0;
    }
  }

  if( bHex && nLen > 16 ) return 0;

  // Convert charater by Charater
  uint64_t retValue = 0;
  for( i=0; i< nLen; i++ ) {
    if( bHex ) {
      retValue <<= 4;
      retValue += h2i(strData[i]);
    } else {
      retValue *= 10;
      retValue += h2i(strData[i]);
    }
  }

  return retValue;
}
