#include <sys/types.h>
typedef unsigned int  uint_t;
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <iostream>

// taglib hrs
#include <taglib/fileref.h>
#include <taglib/tfile.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>

using namespace std;



int main(int argc, char *argv[])
{
    const char* const  data = "陶喆";

    setlocale(LC_ALL, "en_US.utf8");

    size_t  b = strlen(data);
    size_t  n = b*sizeof(wchar_t);
    wchar_t*  w = (wchar_t*)malloc(n);
    size_t  len = mbstowcs(w, data, n);

    free(w);

    TagLib::ID3v2::Tag  t;

    t.setArtist(TagLib::String(data, TagLib::String::UTF8));
    cout << "original utf8='" << data << "' " << b << "bytes, " << len << "mbtye chars " << " as (latin) cstr='" << t.artist().toCString() << "', as (unicode) cstr='" << t.artist().toCString(true) << "'" << endl;

    if (argc == 2) {
	size_t  b1;
	if ( (b1 = strlen(argv[1])) == b && memcmp(data, argv[1], b) == 0) {
	    cout << "input and static data are the same" << endl;
	}

	n = b1*sizeof(wchar_t);
	w = (wchar_t*)malloc(n*sizeof(wchar_t));
	len = mbstowcs(w, argv[1], n);

#if 0
	/* this doesnt work, you must re-encode the input from multibyte to
	 * wchar_t to do processing!
	 */
	t.setComment(TagLib::String(argv[1], TagLib::String::UTF8));
	cout << "original utf8='" << argv[1] << "', as (latin) cstr='" << t.comment().toCString() << "', as (unicode) cstr='" << t.comment().toCString(true) << "'" << endl;
#endif

	t.setTitle(TagLib::String(w, TagLib::String::UTF16BE));
	cout << "original utf8='" << argv[1] << "' " << b1 << "bytes, " << len << "mbyte chars " << "as (latin) cstr='" << t.title().toCString() << "', as (unicode) cstr='" << t.title().toCString(true) << "'" << endl;

	char*  p = argv[1];
	while (*p) {
	    cout << "  '" << *p << "'  [" << (isalnum(*p) ? "" : "non ") << "alpha numeric]\n";
	    ++p;
	}
	cout << endl;
    }

    return 0;
}
