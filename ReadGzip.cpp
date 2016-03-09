//Decoding gzip streams in C++ based on a streambuf based class
//g++ ReadGzip.cpp -lz -o readgzip
#include <zlib.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <string.h>
using namespace std;
const int READ_BUFF_SIZE = 1024*10;
const int DECODE_BUFF_SIZE = 1024*5;

std::string ConcatStr(const char *a, const char *b)
{
	string out(a);
	out.append(b);
	return out;
}

class DecodeGzip : public streambuf
{
protected:
	char readBuff[READ_BUFF_SIZE];
	char decodeBuff[DECODE_BUFF_SIZE];
	streambuf &inStream;
	std::iostream fs;
	z_stream d_stream;
	std::string outBuff;

	streamsize ReturnDataFromOutBuff(char* s, streamsize n);

public:
	DecodeGzip(std::streambuf &inStream);
	virtual ~DecodeGzip();
	streamsize xsgetn (char* s, streamsize n);
	streamsize showmanyc();
};

DecodeGzip::DecodeGzip(std::streambuf &inStream) : inStream(inStream), fs(&inStream)
{
	fs.read(this->readBuff, READ_BUFF_SIZE);

	d_stream.zalloc = (alloc_func)NULL;
	d_stream.zfree = (free_func)NULL;
	d_stream.opaque = (voidpf)NULL;
	d_stream.next_in  = (Bytef*)this->readBuff;
	d_stream.avail_in = (uInt)fs.gcount();
	d_stream.next_out = (Bytef*)this->decodeBuff;
	d_stream.avail_out = (uInt)DECODE_BUFF_SIZE;

	//cout << "read " << d_stream.avail_in << endl;
	int err = inflateInit2(&d_stream, 16+MAX_WBITS);
	if(err != Z_OK)
		throw runtime_error(ConcatStr("inflateInit2 failed: ", zError(err)));

}

DecodeGzip::~DecodeGzip()
{

}

streamsize DecodeGzip::ReturnDataFromOutBuff(char* s, streamsize n)
{
	int lenToCopy = outBuff.size();
	if(n < lenToCopy) lenToCopy = n;
	strncpy(s, outBuff.c_str(), lenToCopy);
	if(lenToCopy > 0)
		outBuff = std::string(&outBuff[lenToCopy], outBuff.size()-lenToCopy);
	return lenToCopy;
}

streamsize DecodeGzip::xsgetn (char* s, streamsize n)
{	
	int err = Z_OK;

	if(outBuff.size() > 0)
		return ReturnDataFromOutBuff(s, n);

	if(d_stream.avail_in == 0)
	{
		if(fs.eof())
			d_stream.avail_in = 0;
		else
		{
			fs.read(this->readBuff, READ_BUFF_SIZE);
			d_stream.next_in  = (Bytef*)this->readBuff;
			d_stream.avail_in = (uInt)fs.gcount();
			//cout << "read " << d_stream.avail_in << endl;
		}
	}

	if(d_stream.avail_in > 0)
	{
		err = inflate(&d_stream, Z_NO_FLUSH);

		if (err != Z_STREAM_END)
		{
			if(err != Z_OK)
				throw runtime_error(ConcatStr("inflate failed: ", zError(err)));

			size_t outLen = DECODE_BUFF_SIZE - d_stream.avail_out;
			outBuff.append(decodeBuff, outLen);
			d_stream.next_out = (Bytef*)this->decodeBuff;
			d_stream.avail_out = (uInt)DECODE_BUFF_SIZE;
			return ReturnDataFromOutBuff(s, n);
		}
	}

	err = inflate(&d_stream, Z_FINISH);
	if(err != Z_OK && err != Z_STREAM_END)
		throw runtime_error(ConcatStr("inflate failed: ", zError(err)));

	err = inflateEnd(&d_stream);
	if(err != Z_OK)
		throw runtime_error(ConcatStr("inflateEnd failed: ", zError(err)));
	
	size_t outLen = DECODE_BUFF_SIZE - d_stream.avail_out;
	outBuff.append(decodeBuff, outLen);
	d_stream.next_out = (Bytef*)this->decodeBuff;
	d_stream.avail_out = (uInt)DECODE_BUFF_SIZE;
	return ReturnDataFromOutBuff(s, n);
}

streamsize DecodeGzip::showmanyc()
{
	if(outBuff.size() > 0)
		return 1;
	return inStream.in_avail() > 1;
}

void Test(streambuf &st)
{
	int testBuffSize = 200;
	char buff[testBuffSize];
	ofstream testOut("testout.txt");
	while(st.in_avail()>0)
	{
		int len = st.sgetn(buff, testBuffSize-1);
		buff[len] = '\0';
		cout << buff;
		testOut << buff;
	}
	testOut.flush();
}

int main()
{
	std::filebuf fb;
	fb.open("test2.txt.gz", std::ios::in);
	class DecodeGzip decodeGzip(fb);

	Test(decodeGzip);
}

