/*
Max3D
Copyright (c) 2008, Mark Sibly
All rights reserved.

Redistribution and use in source and binary forms, with or without
conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of Max3D's copyright owner nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TEXTUREUTIL_H
#define TEXTUREUTIL_H

#include "graphics.h"

typedef CTexture*(*TextureLoader)( const char *path );

class CTextureUtil{
public:
	CTextureUtil();

	//Set texture loader callback
	void SetTextureLoader( TextureLoader loader );

	//Load a texture
	CTexture *LoadTexture( string path );
	
	//Get stock white texture - read only!
	CTexture *WhiteTexture(){ return _whiteTexture; }

	//Get stock black texture - read only!
	CTexture *BlackTexture(){ return _blackTexture; }
	
	//Get stock flat normal texture - read only!
	CTexture *FlatTexture(){ return _flatTexture; }
	
	//Get stock alpha 0 texture - read only!
	CTexture *ZeroTexture(){ return _zeroTexture; }
	
	//Use this to read a texture...
	CTexture *ReadTexture( CStream *stream );

	//And this to write...
	void WriteTexture( CTexture *texture,CStream *stream );

private:
	TextureLoader _textureLoader;

	map<string,CTexture*> _paths;
	map<CTexture*,string> _textures;

	CTexture *_blackTexture;
	CTexture *_whiteTexture;
	CTexture *_flatTexture;
	CTexture *_zeroTexture;
};

#endif
