#include <windows.h> // Header File For Windows
#include <gl\gl.h>	 // Header File For The OpenGL32 Library
#include <gl\glu.h>	 // Header File For The GLu32 Library
#include <vfw.h>	 // Header File For Video For Windows

// LoadAVIFile - loads AVIFile and opens an AVI file.
//
// szfile - filename
// hwnd - window handle
//
void LoadAVIFile(const char *szFile)
{
	LONG hr;
	PAVIFILE pfile;

	AVIFileInit(); // opens AVIFile library

	hr = AVIFileOpen(&pfile, szFile, OF_SHARE_DENY_WRITE, 0L);
	if (hr != 0)
	{
		// Handle failure.
		return;
	}

	//
	// Place functions here that interact with the open file.
	//

	AVIFileRelease(pfile); // closes the file
	AVIFileExit();		   // releases AVIFile library
}