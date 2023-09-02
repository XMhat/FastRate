#define WINVER 0x0400
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINDOWS WINVER

#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>

HWND WinHandle;
HWND WinText;
const char *WinName = "FastRate";

#pragma comment(lib,"kernel32")
#pragma comment(lib,"user32")
#pragma comment(lib,"gdi32")
#pragma comment(lib,"shell32")
#pragma comment(lib,"winmm")
#pragma comment(lib,"bufferoverflowu")

#pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

VOID Exception(BOOL Error, const char *Format, ...)
{
  CHAR FormatBuffer[1024], ErrorBuffer[1024], FinalBuffer[1024];
  PVOID Arguments = &Format + 1;

  if(!wvsprintf(FormatBuffer, Format, (va_list)Arguments))
    wsprintf(FormatBuffer, "Unspecified error");

  if(Error && FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(),
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ErrorBuffer,
       sizeof(ErrorBuffer), 0))
    wsprintf(FinalBuffer, "%s. %s", FormatBuffer, ErrorBuffer);
  else
    wsprintf(FinalBuffer, "%s.", FormatBuffer);

  MessageBox(WinHandle, FinalBuffer, WinName, MB_ICONSTOP);
  ExitProcess(1);
}

VOID UpdateLabel()
{
  CHAR Buffer[1024];
  lstrcpy(Buffer, "FastRate compiled  " __TIMESTAMP__ ".\n\n"
    "Click the X button or press ALT+F4 to quit.\n"
    "Click the _ button to minimize to tray.");
  SetWindowText(WinText, Buffer);
}

INT CALLBACK WinProc(HWND H, UINT M, WPARAM W, LPARAM L)
{
  switch(M)
  {
    case WM_APP:
      if(L == WM_LBUTTONDBLCLK)
      {
        ShowWindow(WinHandle, SW_RESTORE);
        SetForegroundWindow(WinHandle);
      }
      break;
    case WM_ACTIVATE:
      if(HIWORD(W))
        ShowWindow(WinHandle, SW_HIDE);
    case WM_SETFOCUS:
      UpdateLabel();
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(H, M, W, L);
  }

  return 0;
}

VOID WinMainCRTStartup(VOID)
{
  HINSTANCE I = GetModuleHandle(NULL);
  NOTIFYICONDATA NID;
  WNDCLASS Wc;
  MSG Msg;
  INT GmVal;
  HFONT WinFont;
  HANDLE Mutex;

  Mutex = CreateMutex(0, 1, WinName);
  if(Mutex == NULL)
    Exception(1, "Failed to create mutex");
  if(GetLastError() == ERROR_ALREADY_EXISTS)
    Exception(0, "Program already running");
  if(timeBeginPeriod(1) != TIMERR_NOERROR)
    Exception(1, "Kernel refresh rate could not be adjusted");

  Wc.style = CS_HREDRAW | CS_VREDRAW;
  Wc.lpfnWndProc = (WNDPROC)WinProc;
  Wc.hInstance = I;
  Wc.hIcon = LoadIcon(I, MAKEINTRESOURCE(1));
  Wc.hCursor = LoadCursor(0, IDC_ARROW);
  Wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
  Wc.lpszClassName = WinName;
  Wc.lpszMenuName = 0;
  Wc.cbClsExtra = Wc.cbWndExtra = 0;

  if(!RegisterClass(&Wc))
    Exception(1, "Failed to register window class");
  WinHandle = CreateWindow(Wc.lpszClassName, Wc.lpszClassName, WS_POPUP |
    WS_DLGFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT,
    CW_USEDEFAULT, 320, 240, 0, 0, I, 0);
  if(WinHandle == NULL)
    Exception(1, "Failed to create window");
  WinFont = CreateFont(13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "MS Shell Dlg");
  if(WinFont == NULL)
    Exception(1, "Failed to create shell dialog font");
  WinText = CreateWindow("STATIC", 0, WS_CHILD | WS_VISIBLE, 10, 10, 300, 200,
    WinHandle, 0, I, 0);
  if(WinText == NULL)
    Exception(1, "Failed to create text object");

  SendMessage(WinText, WM_SETFONT, (WPARAM)WinFont, 0);

  NID.cbSize = sizeof(NID);
  NID.uID = 1;
  NID.hIcon = (HICON)LoadImage(I, MAKEINTRESOURCE(1), IMAGE_ICON, 16, 16, 0);
  NID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  NID.hWnd = WinHandle;
  NID.uCallbackMessage = WM_APP;

  lstrcpy(NID.szTip, WinName);

  if(!Shell_NotifyIcon(NIM_ADD, &NID))
    Exception(1, "Failed to create system tray icon");

  for(;;)
  {
    GmVal = GetMessage(&Msg, 0, 0, 0);
    if(GmVal == 0 || GmVal == -1)
      break;
    TranslateMessage(&Msg);
    DispatchMessage(&Msg);
  }

  Shell_NotifyIcon(NIM_DELETE, &NID);
  timeEndPeriod(1);
  ReleaseMutex(Mutex);
  DeleteObject(WinFont);
  DeleteObject(NID.hIcon);
  DeleteObject(Wc.hIcon);
  DeleteObject(Wc.hCursor);
  UnregisterClass(WinName, I);

  ExitProcess(0);
}
