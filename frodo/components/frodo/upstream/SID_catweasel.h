/*
 *  SID_linux.h - 6581 emulation, Catweasel specific stuff
 *
 *  Frodo Copyright (C) Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <unistd.h>
#include <fcntl.h>

// Catweasel ioctls (included here for convenience)
#include <sys/ioctl.h>
#define CWSID_IOCTL_TYPE ('S')
#define CWSID_IOCTL_RESET        _IO(CWSID_IOCTL_TYPE, 0)
#define CWSID_IOCTL_CARDTYPE	 _IOR(CWSID_IOCTL_TYPE, 4, int)
#define CWSID_IOCTL_PAL          _IO(CWSID_IOCTL_TYPE, 0x11)
#define CWSID_IOCTL_NTSC         _IO(CWSID_IOCTL_TYPE, 0x12)
#define CWSID_IOCTL_DOUBLEBUFFER _IOW(CWSID_IOCTL_TYPE, 0x21, int)
#define CWSID_IOCTL_DELAY        _IOW(CWSID_IOCTL_TYPE, 0x22, int)
#define CWSID_MAGIC 0x100


/*
 *  Renderer for Catweasel card
 */

// Renderer class
class CatweaselRenderer : public SIDRenderer {
public:
	CatweaselRenderer();
	virtual ~CatweaselRenderer();

	void Reset() override;
	void EmulateLine() override {}
	void WriteRegister(uint16_t adr, uint8_t byte) override;
	void NewPrefs(const Prefs *prefs) override {}
	void Pause() override {}
	void Resume() override {}

private:
	int cwsid_fh; // Catweasel device file handle
};

// Constructor: Open Catweasel device and reset SID
CatweaselRenderer::CatweaselRenderer()
{
	cwsid_fh = open("/dev/sid", O_WRONLY);
	if (cwsid_fh >= 0) {
		int i;
		if (ioctl(cwsid_fh, CWSID_IOCTL_CARDTYPE, &i) < 0 || i != CWSID_MAGIC) {
			close(cwsid_fh);
			cwsid_fh = -1;
		} else {
			ioctl(cwsid_fh, CWSID_IOCTL_RESET);
			ioctl(cwsid_fh, CWSID_IOCTL_DOUBLEBUFFER, 0);
		}
	}

	Reset();
}

// Destructor: Reset SID and close Catweasel device
CatweaselRenderer::~CatweaselRenderer()
{
	Reset();

	if (cwsid_fh >= 0) {
		close(cwsid_fh);
		cwsid_fh = -1;
	}
}

// Reset SID
void CatweaselRenderer::Reset()
{
	if (cwsid_fh >= 0) {
		uint8_t zero = 0;
		ioctl(cwsid_fh, CWSID_IOCTL_RESET);
		lseek(cwsid_fh, 24, SEEK_SET);
		write(cwsid_fh, &zero, 1);
	}
}

// Write to register
void CatweaselRenderer::WriteRegister(uint16_t adr, uint8_t byte)
{
	if (cwsid_fh >= 0 && adr < 0x1a) {
		lseek(cwsid_fh, adr, SEEK_SET);
		write(cwsid_fh, &byte, 1);
		lseek(cwsid_fh, adr, SEEK_SET);
		write(cwsid_fh, &byte, 1);
	}
}
