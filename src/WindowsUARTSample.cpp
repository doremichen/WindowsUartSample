//============================================================================
// Name        : WindowsUARTSample.cpp
// Author      : Adam
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <iostream>
#include <conio.h>


using namespace std;

#define SIZEOFBUF(BUF) sizeof(BUF)/sizeof(BUF[0])

HANDLE	hPort1;
HANDLE  ghReadThread;

DCB		PortDCB;
LPCTSTR comNum;

DWORD WINAPI ReadThreadProc(LPVOID);


int SetCOMConfig(HANDLE Port)
{
	cout << "[" << __FUNCTION__ << "]" << endl;
	int ret = 1;   //success
	int bStatus = 0;

	// Set Port parameters.
	// Make a call to GetCommState() first in order to fill
	// the comSettings structure with all the necessary values.
	// Then change the ones you want and call SetCommState().
	GetCommState(Port, &PortDCB);
	PortDCB.BaudRate = 460800;
	PortDCB.StopBits = ONESTOPBIT;
	PortDCB.ByteSize = 8;
	PortDCB.Parity   = NOPARITY;
	PortDCB.fParity  = FALSE;
	bStatus = SetCommState(Port, &PortDCB);

	if(bStatus == 0) {
		cerr << "[" << __FUNCTION__ << "]" << " set port config fail.." << endl;
		ret = 0;
	}

	return ret;
}


int setTimeOutConfig(HANDLE Port)
{
	cout << "[" << __FUNCTION__ << "]" << endl;
	int ret = 1; //success
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout        = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier    = 0;
	timeouts.ReadTotalTimeoutConstant    = 0;
	timeouts.WriteTotalTimeoutMultiplier    = 0;
	timeouts.WriteTotalTimeoutConstant    = 0;

	if (!SetCommTimeouts(Port, &timeouts)) {
		ret = 0;
		cerr << "[" << __FUNCTION__ << "]" << " set comm time fail..." << endl;
	}
	else {
		cout << "[" << __FUNCTION__ << "]" << " set comm time success..." << endl;
	}

	return ret;
}

HRESULT OpenPort()
{
	HRESULT ret = E_FAIL;
	char Text[10] = {0};
	char prefix[] = "\\\\.\\";
	string PortNum;
	int status_config = 0;
	int status_timeconf = 0;
	int status_setCommMask = 0;
	DWORD dwRThreadID;

	cout << "[ " << __FUNCTION__ << " ]: Please input Port num: " << endl;

	while(true) {
		cin >> PortNum;

		if(PortNum.compare("") > 0) {

			cout << "[" << __FUNCTION__ << "]" << " portNum: " << PortNum << endl;
			cout << "[" << __FUNCTION__ << "]" << " str len portNum: " << PortNum.length() << endl;
			int strlength = PortNum.length();
			if(strlength < 5) {  //com number: 0~9
				PortNum.copy(Text, strlength);
			}
			else {                      //com number: 10~xx
				memcpy(Text, prefix, sizeof(prefix));
				PortNum.copy(Text+4, strlength);
			}

			cout << "[" << __FUNCTION__ << "]" << " Text: " << Text << endl;

			comNum = (LPCTSTR)Text;

			hPort1 = CreateFile(
						comNum,
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED,
						0);
			printf("[%s] : CreateFile to COM port [%p]\n", __FUNCTION__, hPort1);

			if(hPort1 == INVALID_HANDLE_VALUE) {
				cerr << "[" << __FUNCTION__ << "]" << " Open Com port fail.." << endl;
				break;
			}

			status_setCommMask = SetCommMask(hPort1, EV_RXCHAR|EV_TXEMPTY);

			if(!status_setCommMask) {
				cerr << "[" << __FUNCTION__ << "]" << " set comm mask.." << endl;
				break;
			}


			status_config = SetCOMConfig(hPort1);

			if(status_config == 1) {
				cout << "[" << __FUNCTION__ << "]" << " set config success.." << endl;
			}
			else {
				cerr << "[" << __FUNCTION__ << "]" << " set config fail.." << endl;
				break;
			}

			status_timeconf = setTimeOutConfig(hPort1);

			if(status_timeconf == 1) {
				cout << "[" << __FUNCTION__ << "]" << " set time config success.." << endl;
			}
			else {
				cerr << "[" << __FUNCTION__ << "]" << " set time config fail.." << endl;
				break;
			}

			cout << "[" << __FUNCTION__ << "]" << " Open Com port success " << endl;


			//create read thread
			ghReadThread = CreateThread(
						NULL,
						0,
						ReadThreadProc,
						NULL,
						0,
						&dwRThreadID);

			if(ghReadThread == NULL) {
				cerr << "[" << __FUNCTION__ << "]" << " creat reader thread fail.." << endl;
				break;
			}

			cout << "[" << __FUNCTION__ << "]" << " creat reader thread success " << endl;
			ret = S_OK;
			break;
		}
	}

	return ret;
}


//HRESULT Write (const char* data,DWORD dwSize)
HRESULT Write ()
{
	printf("[%s] :\n", __FUNCTION__);
//	printf("[%s] : data {%s}, dwSize {%lu}\n", __FUNCTION__, data, dwSize);
	HRESULT ret = E_FAIL;
	int iRet = 0;
	OVERLAPPED ov;
	memset(&ov, 0, sizeof(ov));

	ov.hEvent = CreateEvent(0, true, 0, 0);
	DWORD dwByteWritten = 0;

	printf("[%s] : start WriteFile to COM port [%p]\n", __FUNCTION__, hPort1);
	char szBuffer[ ] = {0x01, 0x0d};


//	iRet = WriteFile(hPort1, data, dwSize, &dwByteWritten, &ov);
//	iRet = WriteFile(hPort1, data, dwSize, &dwByteWritten, NULL);
	iRet = WriteFile(hPort1, szBuffer, SIZEOFBUF(szBuffer), &dwByteWritten, &ov);

	if(iRet == 0) {
		printf("[%s] : WaitForSingleObject\n", __FUNCTION__);
		WaitForSingleObject(ov.hEvent, INFINITE);
	}

	CloseHandle(ov.hEvent);
//	string szData(data);
	ret = S_OK;

//	printf("[%s] : Writing {%s}, length {%d} \n", __FUNCTION__, szData.c_str(), szData.length());
	return ret;
}

DWORD WINAPI ReadThreadProc(LPVOID)
{
	bool abContinue = true;
	DWORD dwEventMask = 0;

	OVERLAPPED ov;
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = CreateEvent(0, true, 0, 0);
	HANDLE arHandles;

	DWORD dwWait;

	while(abContinue) {
		cout << "[" << __FUNCTION__ << "]" << "WaitCommEvent" << endl;
		BOOL abRet = WaitCommEvent(hPort1, &dwEventMask, &ov);
		if(!abRet) {
			if(GetLastError() == ERROR_IO_PENDING) {
				cerr << "[" << __FUNCTION__ << "]" << " ERROR_IO_PENDING.." << endl;
			}
		}

		arHandles = ov.hEvent;
		cout << "[" << __FUNCTION__ << "]" << "WaitForSingleObject" << endl;
		dwWait = WaitForSingleObject(arHandles, INFINITE);
        cout << "[" << __FUNCTION__ << "]" << "dwWait = " << dwWait << endl;
		if(dwWait == WAIT_OBJECT_0) {
			DWORD dwMask;
			if(GetCommMask(hPort1, &dwMask)) {
				if(dwMask == EV_TXEMPTY) {
					cout << "[" << __FUNCTION__ << "]" << "Data sent" << endl;
					ResetEvent(ov.hEvent);
					continue;
				}
			}

			//read data here
			try {
				BOOL readRet = false;

				DWORD dwByteRead = 0;
				OVERLAPPED ovRead;
				memset(&ovRead, 0, sizeof(ovRead));
				ovRead.hEvent = CreateEvent(0, true, 0, 0);

				do {
					ResetEvent(ovRead.hEvent);
					char szTmp[1];
					int iSize  = sizeof(szTmp);
					memset(szTmp,0,sizeof szTmp);

					readRet = ReadFile(hPort1, szTmp, iSize, &dwByteRead, &ovRead);

					if(!readRet) {
						abContinue = FALSE;
						cerr << "[" << __FUNCTION__ << "]" << " read data fail.." << endl;
						break;
					}

					if(dwByteRead > 0) {
						printf("[%s] : Read {%x}, length {%d} \n", __FUNCTION__, szTmp[0], (int)dwByteRead);
					}
				}while(dwByteRead);
				CloseHandle(ovRead.hEvent);

			}
			catch(...) {

			}

			ResetEvent ( ov.hEvent );
		}
	}


	return 0;
}


int main(int argc, char *argv[])
{

	int index = 0;
	string input;

	index++;

	while(true) {
		cout << "[ " << __FUNCTION__ << " ]: please input command: " << endl;
		cin >> input;

		if(!input.compare("exit")) {
			goto Exit;
		}
		else if(!input.compare("open")) {
			OpenPort();
		}
		else if(!input.compare("test")) {
			Write();
//			string testStr;
//			cin >> testStr;
//
//			printf("%s\n", testStr.c_str());

//			Write(testStr.c_str(), testStr.length());


		}
	}


Exit:

	return 0;
}


