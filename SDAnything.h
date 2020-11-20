#include <SD.h>

/*
 * 
 */
template <class T> int SD_writeAnything(char* FileName, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    File FileObject;
    FileObject = SD.open(FileName, FILE_WRITE);
    
    // if the file opened okay, write to it, else return -1
    if (FileObject) {
      FileObject.seek(0);
        for (i = 0; i < sizeof(value); i++)
          FileObject.write(p++,1);
      
      // close the file:
      FileObject.close();
      return i;
    } else return -1;
}

template <class T> int SD_reWriteAnything(char* FileName, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    SD.remove(FileName);
    File FileObject;
    FileObject = SD.open(FileName, FILE_WRITE);
    
    // if the file opened okay, write to it, else return -1
    if (FileObject) {
      FileObject.seek(0);
        for (i = 0; i < sizeof(value); i++)
          FileObject.write(p++,1);
      
      // close the file:
      FileObject.close();
      return i;
    } else return -1;
}

template <class T> int SD_readAnything(char* FileName, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    File FileObject;
    FileObject = SD.open(FileName);
    
    // if the file opened okay, write to it, else return -1
    if (FileObject) {
      for (i = 0; i < sizeof(value); i++)
        *p++ = FileObject.read();
      
      // close the file:
      FileObject.close();
      return i;
    } else return -1;
}