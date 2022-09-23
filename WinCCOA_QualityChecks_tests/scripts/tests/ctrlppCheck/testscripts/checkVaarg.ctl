// start options:

// error id: va_start_referencePassed
// Using reference 'refParam' as parameter for va_start() results in undefined behaviour. (CWE: 758)

void va_start_referencePassed(va_list &refParam)
{
  va_start(refParam);
}

// error id: va_list_usedBeforeStarted
// va_list 'refParam' used before va_start() was called. (CWE: 664)
void main()
{
  va_list refParam;
  va_start_referencePassed(refParam);
}


// error id: checkLibraryFunctionArgCount
// The function has invalid count of arguments: va_start() (CWE: 628)
void checkLibraryFunctionArgCount(...)
{
  va_start();
  va_start(1, 2);

  va_list list;
  va_start(list, 2);

  // typical error - cpp usage, but not ctrl usage
  int i;
  va_start(i, list);
}


// error id: va_end_missing
// va_list 'refParam' was opened but not closed by va_end(). (CWE: 664)
void va_end_missing(...)
{
  va_list list;
  int count = va_start(list);
}


// error id: va_start_subsequentCalls
// va_start() called subsequently on 'list' without va_end() in between. (CWE: 664)
void va_start_subsequentCalls(...)
{
    
  va_list list;
  int count = va_start(list);
  // ....
  count = va_start(list);

  // close list
  va_end(list);
  // this is not error, because list is close at line before
  count = va_start(list);

  // close it twice.
  // it produce error: va_list_usedBeforeStarted
  // the message can be more precise, but it is enought just now.
  va_end(list);
  va_end(list);
}
