char inbyte(void);
int outbyte(char c);


int main()
{
  outbyte ('&');
  outbyte ('@');
  outbyte ('$');
  outbyte ('%');

  /* whew, we made it */
  
  print ("\r\nDone...");

  return 0;
}
