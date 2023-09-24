// start options:
// error id: unreadVariable
// Variable is assigned a value that is never used: 's'

class C
{
  public int count;
  public int getCount()
  {
    return count;
  }
}

void main()
{
  string s = "";
  s = "abc"; // error never used string value

  int i = 1;
  i = 2; // never used int value

  C c;
  c.count = 0; // never used member of class

  C c2;
  c2.count = 0; // this is not a error. The instance c2 of class C are used in next line.
  int i2 = c2.getCount();

  shape sh = getShape("abc");
  sh.visible = TRUE; // shapes are ignored, 
}

