#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>

#include <iostream>

// taglib hrs
#include <taglib/tfile.h>
#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2header.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
#include <taglib/tlist.h>
#include <taglib/textidentificationframe.h>
#include <taglib/commentsframe.h>

using namespace std;


const char*  _argv0 = NULL;
bool  _verbose = false;

const TagLib::String::Type  INTNL_STR_ENC = TagLib::String::UTF16BE;



#define MP3_TAG_ERR(m)  cerr << _argv0 << ": [error]  " << m << endl;
#define MP3_TAG_WARN(m)  cout << _argv0 << ": [warn]  " << m << endl;
#define MP3_TAG_NOTICE(m)  cout << _argv0 << ": " << m << endl;

#define MP3_TAG_WARN_VERBOSE(m)  if (_verbose)  { cout << _argv0 << ": [warn]  " << m << endl; }
#define MP3_TAG_NOTICE_VERBOSE(m)  if (_verbose)  { cout << _argv0 << ":  " << m << endl; }

#ifdef DEBUG
#define MP3_TAG_DEBUG(m)  cout << _argv0 << "; [DEBUG]  " << m << endl;
#else
#define MP3_TAG_DEBUG(m)  
#endif

void  _usage()
{
    cout << "usage: " << _argv0 << " [OPTION]... [mp3 FILES]" << endl
	 << endl
	 << "  [tag encoding options]" << endl
	 << "       -e  encoding  = utf16be [latin1|utf8|utf16|utf16be|utf18le]" << endl
	 << endl
	 << "  [tagging options]" << endl
	 << "       -t  title" << endl
	 << "       -a  artist" << endl
	 << "       -A  album" << endl
	 << "       -c  comment" << endl
	 << "       -g  genre" << endl
	 << "       -y  year" << endl
	 << "       -T  track" << endl
	 << "       -1             add ID3v1 tag" << endl
	 << "       -2             add ID3v2 tag" << endl
	 << endl
	 << "  [maintainence options]" << endl
	 << "       -l             list tags (exclusive maintanence option" << endl
	 << "       -D [1|2]       delete ID3 tag" << endl
	 << "       -C             clean tags, leaving only basic info" << endl
	 << "       -M encoding    parse current tags and convert from -M <E> -e <E'>" << endl
	 << "               [warn] will damage tags if you get the E wrong!" << endl
	 << endl
	 << "  [misc options]" << endl
	 << "       -V             verbose" << endl;

    exit(1);
}


TagLib::String  _cnvrt(const char* data_)
{
    const size_t  n = strlen(data_)*sizeof(wchar_t);
    if (n == 0) {
        return TagLib::String::null;
    }

    if (INTNL_STR_ENC == TagLib::String::Latin1) {
        // nice and simple
	MP3_TAG_DEBUG("encoded as [" << INTNL_STR_ENC << "]  '" << data_ << "'");
	return TagLib::String(data_);
    }

    wchar_t*      w = new wchar_t[n];
    memset(w, 0, n);
    const size_t  len = mbstowcs(w, data_, n);

#ifdef DEBUG
    char  p[MB_CUR_MAX+1];
    for (int c=0; c<len; c++)
    {
	/* re-convert from wide character to multibyte character */
	int x = wctomb(p, w[c]);

	/* One multibyte character may be two or more bytes.
	 * Thus "%s" is used instead of "%c".
	 */
	if (x>0) p[x]=0;
	MP3_TAG_DEBUG("multibyte char[" << c << "] = '" << p << "'  (" << x << " bytes)");
    }
#endif

    const TagLib::String  s(w, INTNL_STR_ENC);
    MP3_TAG_DEBUG("encoded as [" << INTNL_STR_ENC << "]  '" << data_ << "', unicode='" << s.toCString(true) << "', latin1='" << s.toCString() << "'");
    delete []  w;
    return s;
}


const char*  _strrep(TagLib::String  str_, TagLib::String* ptr_=NULL)
{
    /* so fucking stupid..
     */
    const char*  tmp = "";
    if (str_ == TagLib::String::null) {
         return tmp;
    }
    else {
	if (ptr_) {
	    *ptr_ = str_;
	    return ptr_->toCString(true);
	}
	else {
	    static TagLib::String  tmp;
	    tmp = str_;
	    return tmp.toCString(true);
	}
    }
}


/* placeholders only */
class IFlds
{
  public:
    IFlds() :
	artist(NULL),
	album(NULL),
	title(NULL),
	comment(NULL),
	genre(NULL),
	yr(NULL),
	trackno(NULL)
    { }

    const char*  artist;   // TPE1
    const char*  album;    // TALB or TOAL
    const char*  title;    // TIT2
    const char*  comment;  // COMM
    const char*  genre;    // TCON
    const char*  yr;       // TDRC or TYER or TORY
    const char*  trackno;  // TRCK

    operator bool() const 
    { return artist || album || title || comment || genre || yr || trackno; }

    void  strip()
    {
        const char**  a[] = { &artist, &album, &title, &comment, &genre, &yr, &trackno , NULL };

	const char***  p = a;
	while (*p)
	{
	     if (**p) {
		 char*  x = (char*)**p;
		 char*  y = x;

		 while (*x) {
		     ++x;
		 }

		 /* y = start, x = end */
		 if (y == x) {
		 }
		 else 
		 {
		     --x;
		     /* get rid of trailing... */
		     while (x > y && isspace(*x)) {
			 --x;
		     }
		     *++x = NULL;

		     /* ...and leading */
		     while (y < x && isspace(*y)) {
			 ++y;
		     }

		     **p = (y == x) ? NULL : y;
		 }
	     }
	     ++p;
	}
    }

    void  populate(const TagLib::Tag* tag_)
    {
	artist = _strrep(tag_->artist(), &a);
	title = _strrep(tag_->title(), &t);
	album = _strrep(tag_->album(), &A);
	genre = _strrep(tag_->genre(), &g);

	sprintf(T, "%ld", tag_->track());
	sprintf(y, "%ld", tag_->year());
	trackno = T;
	yr = y;
    }

  private:
    TagLib::String  a,t,A,g;
    char  y[5];
    char  T[3];
};


void  _tagFrme(TagLib::ID3v2::Tag*  tag_, TagLib::ID3v2::Frame* frme_, const char* data_)
{
    tag_->removeFrames(frme_->frameID());
    if (strlen(data_)) {
	frme_->setText(_cnvrt(data_));
    }
    tag_->addFrame(frme_);
}

TagLib::String::Type  _overrideEncLatin(const char* data_, TagLib::String::Type enc_)
{
    const char*  p = data_;
    while (*p) {
	if (!isascii(*p++)) {
	    return enc_;
	}
    }
    return TagLib::String::Latin1;
}

void  _tag(TagLib::ID3v2::Tag*  tag_, const IFlds&  flds_, TagLib::String::Type  enc_)
{
    if (flds_.genre) {
	if (strlen(flds_.genre) == 0) {
	    tag_->removeFrames("TCON");
	}
	else {
	    tag_->setGenre(_cnvrt(flds_.genre));
	}
    }
    if (flds_.yr)       tag_->setYear    ((unsigned)atol(flds_.yr));

#if 0
    if (flds_.artist)   _tagFrme(tag_, new TagLib::ID3v2::TextIdentificationFrame("TPE1", enc_), flds_.artist);
    if (flds_.album)    _tagFrme(tag_, new TagLib::ID3v2::TextIdentificationFrame("TALB", enc_), flds_.album);
    if (flds_.title)    _tagFrme(tag_, new TagLib::ID3v2::TextIdentificationFrame("TIT2", enc_), flds_.title);
#else
    if (flds_.artist)   _tagFrme(tag_, new TagLib::ID3v2::TextIdentificationFrame("TPE1", _overrideEncLatin(flds_.artist, enc_)), flds_.artist);
    if (flds_.album)    _tagFrme(tag_, new TagLib::ID3v2::TextIdentificationFrame("TALB", _overrideEncLatin(flds_.album, enc_)), flds_.album);
    if (flds_.title)    _tagFrme(tag_, new TagLib::ID3v2::TextIdentificationFrame("TIT2", _overrideEncLatin(flds_.title, enc_)), flds_.title);
#endif

    if (flds_.trackno)  _tagFrme(tag_, new TagLib::ID3v2::TextIdentificationFrame("TRCK", TagLib::String::Latin1), flds_.trackno);

    if (flds_.comment) {
	if (strlen(flds_.comment) == 0) {
	    tag_->removeFrames("COMM");
	}
	else {
	    _tagFrme(tag_, new TagLib::ID3v2::CommentsFrame(enc_), flds_.comment);
	}
    }
}

void  _tag(TagLib::Tag*  tag_, const IFlds&  flds_)
{

    if (flds_.artist)     tag_->setArtist  (_cnvrt(flds_.artist));
    if (flds_.album)      tag_->setAlbum   (_cnvrt(flds_.album));
    if (flds_.title)      tag_->setTitle   (_cnvrt(flds_.title));
    if (flds_.comment)    tag_->setComment (_cnvrt(flds_.comment));
    if (flds_.genre)      tag_->setGenre   (_cnvrt(flds_.genre));

    if (flds_.yr)         tag_->setYear    (atol(flds_.yr));
    if (flds_.trackno)    tag_->setTrack   (atol(flds_.trackno));
}

void  _displayID3v1Tag(const TagLib::ID3v1::Tag*  tag_, const char* file_, const char* hdr_)
{
    if (tag_->isEmpty()) {
        return;
    }
    cout << "id3v1 tag: " << file_ << "  (not multibyte safe)\n"
	 << "    Artist  : " << _strrep(tag_->artist()) << endl
         << "    Title   : " << _strrep(tag_->title()) << "      Track : " << tag_->track() << endl
	 << "    Album   : " << _strrep(tag_->album()) << endl
	 << "    Comment : " << _strrep(tag_->comment()) << endl
	 << "    Year    : " << tag_->year() << "    " << "Genre   : " << _strrep(tag_->genre()) << endl;
}


const char*  _id3v2TxtEnc(const TagLib::ID3v2::Frame* f_)
{
    static const char *encodings[] = { "Latin1", "UTF16", "UTF16BE", "UTF8", "UTF16LE" };

    if (f_->frameID().startsWith("T")) {
	const TagLib::ID3v2::TextIdentificationFrame*  txtf = NULL;
	if ( (txtf = dynamic_cast<const TagLib::ID3v2::TextIdentificationFrame*>(f_)) == NULL) {
	    return "";
	}

	return encodings[int(txtf->textEncoding())];
    }

    if ( f_->frameID() == "COMM") {
	const TagLib::ID3v2::CommentsFrame*  cmmf = NULL;
    	if ( (cmmf = dynamic_cast<const TagLib::ID3v2::CommentsFrame*>(f_)) == NULL) {
	    return "";
	}

	return encodings[int(cmmf->textEncoding())];
    }
    return "";
}

void  _displayID3v2Tag(const TagLib::ID3v2::Tag*  tag_, const char* file_, const char* hdr_)
{
    TagLib::ID3v2::Header*  hdr = tag_->header();
    short  version = hdr ? hdr->majorVersion() : 0;
    cout << "id3v2." << version << " tag : " << file_;
    if (tag_->isEmpty()) {
        cout << "  <EMPTY>" << endl;
        return;
    }
    cout << endl
	 << "    Artist  : " << _strrep(tag_->artist()) << endl
         << "    Title   : " << _strrep(tag_->title()) << "      Track : " << tag_->track() << endl
	 << "    Album   : " << _strrep(tag_->album()) << endl
	 << "    Comment : " << _strrep(tag_->comment()) << endl
	 << "    Year    : " << tag_->year() << "    "
	 << "Genre   : " << _strrep(tag_->genre()) << endl;

    if (_verbose) {
	struct stat  st;
	stat(file_, &st);
	cout << "  tagsz: " << hdr->tagSize() << "  filesz: " << st.st_size << endl;

	const TagLib::ID3v2::FrameList&  framelist = tag_->frameList(); 
	for (TagLib::ID3v2::FrameList::ConstIterator i=framelist.begin(); i!=framelist.end(); ++i)
	{
	    //cout << "    " << (*i)->frameID() << ": " << (*i)->toString().toCString(true) << endl;

	    const TagLib::ID3v2::Frame*  f = *i;
	    cout << "    " << f->frameID() << " " << _id3v2TxtEnc(f) << ": '" << f->toString().toCString(true) << "'" << endl;
	}
    }
}

void  _displayAPETag(const TagLib::APE::Tag*  tag_, const char* file_, const char* hdr_)
{
    cout << "APE tag: " << file_;
    if (tag_->isEmpty()) {
        cout << "  <EMPTY>" << endl;
        return;
    }
    cout << endl
	 << "    Artist  : " << _strrep(tag_->artist()) << endl
         << "    Title   : " << _strrep(tag_->title()) << "      Track : " << tag_->track() << endl
	 << "    Album   : " << _strrep(tag_->album()) << endl
	 << "    Comment : " << _strrep(tag_->comment()) << endl
	 << "    Year    : " << tag_->year() << "    " << "Genre   : " << _strrep(tag_->genre()) << endl;
}


TagLib::String::Type  _parseEnc(const char* optarg, TagLib::String::Type dflt_)
{
    return strcasecmp(optarg, "utf8") == 0 ?  TagLib::String::UTF8 :
	      strncasecmp(optarg, "latin", strlen("latin")) == 0 ? TagLib::String::Latin1 :
		  strcasecmp(optarg, "utf16") == 0 ? TagLib::String::UTF16 :
		      strcasecmp(optarg, "utf16be") == 0 ? TagLib::String::UTF16BE :
			  strcasecmp(optarg, "utf16le") == 0 ? TagLib::String::UTF16LE :
	  dflt_;
}

extern int    optind;
extern char*  optarg;


int main(int argc, char *argv[])
{
    _argv0 = basename(argv[0]);

    int   rmtag = 0;
    int   svtag = 0;
    bool  list = false;
    bool  clean = false;
    bool  mbconvert = false;
    bool  prsvtm = false;

    /* what we're encoding from */
    TagLib::String::Type  mbenc = TagLib::String::UTF8;

    IFlds  iflds;

    /* this is encoding for the frames, default it to utf8 */
    TagLib::String::Type  enc = TagLib::String::UTF8;

    const char*  l;
    if ( (l = setlocale(LC_ALL, "en_US.UTF-8")) == NULL) {
        MP3_TAG_NOTICE_VERBOSE("locale set");
    }

    int c;
    while ( (c = getopt(argc, argv, "e:12hla:t:A:y:c:T:g:D:VM:Cp")) != EOF)
    {
	switch (c) {
	    case 'e':
	    {
		enc = _parseEnc(optarg, TagLib::String::UTF8);
	    } break;

	    case 'l':  list = true;  break;

	    case '1':  svtag |= TagLib::MPEG::File::ID3v1;  break;
	    case '2':  svtag |= TagLib::MPEG::File::ID3v2;  break;

	    case 't':  iflds.title = optarg;  break;
	    case 'a':  iflds.artist = optarg;  break;
	    case 'A':  iflds.album = optarg;  break;
	    case 'y':  iflds.yr = optarg;  break;
	    case 'c':  iflds.comment = optarg;  break;
	    case 'T':  iflds.trackno = optarg;  break;
	    case 'g':  iflds.genre = optarg;  break;

	    case 'D':
	    {
	        rmtag |= optarg[0] == '1' ? TagLib::MPEG::File::ID3v1 : rmtag;
	        rmtag |= optarg[0] == '2' ? TagLib::MPEG::File::ID3v2 : rmtag;
	        rmtag |= optarg[0] == 'A' ? TagLib::MPEG::File::APE : rmtag;
		
		if (strcmp(optarg, "12") == 0 || strcmp(optarg, "21") == 0) {
		    rmtag = TagLib::MPEG::File::AllTags;
		}
	    } break;

	    case 'V':  _verbose = true;  break;
	    case 'p':  prsvtm = true;  break;

	    case 'C':  clean = true;  break;
	    case 'M':
	    {
		mbconvert = true;
		mbenc = _parseEnc(optarg, TagLib::String::Latin1);
	    } break;

	    case 'h':
	    default: _usage();
	}
    }

    if ( !(optind < argc) ) {
        MP3_TAG_ERR("no files specified");
	_usage();
    }

    if (rmtag == 0 && svtag == 0) {
	/* default it... */
	svtag = TagLib::MPEG::File::ID3v2;
    }
    
    if ( (!list && !iflds && !rmtag && !clean && !mbconvert) || (iflds && mbconvert)) {
	//_usage();
        list = true;
    }
    iflds.strip();

    if (!list) {
	// dont do this, set it based on each frame as we need
	//TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(enc);
    }


    struct stat  st;
    const char*  f;

    int  i = optind;
    while (i < argc)
    {
	f = argv[i++];

        if (stat(f, &st) < 0) {
	    MP3_TAG_WARN("'" << f << "' unable to determine file, ignoring - " << strerror(errno));
	    continue;
	}

	if ( ! (st.st_mode & (S_IFREG | S_IFLNK | S_IRUSR | S_IWUSR)) ) {
	    MP3_TAG_WARN("'" << f << "' is not valid file, ignoring");
	    continue;
	}


	// and now, for the work...
	if (strstr(f, ".mp3") == NULL) {
	    MP3_TAG_WARN("'" << f << "' doesn't appear to be an mp3 file, ignoring");
	    continue;
	}


	TagLib::Tag*  tag = NULL;

	/* listing takes precidence above the mod actions
	 */
	if (list)
	{
	    TagLib::MPEG::File  tlf(f, false);
	    if ( (tag = tlf.ID3v1Tag(false)) ) {
	        _displayID3v1Tag((const TagLib::ID3v1::Tag*)tag, f, "ID3v1 tag");
	    }
	    if ( (tag = tlf.ID3v2Tag(false)) ) {
	        _displayID3v2Tag((const TagLib::ID3v2::Tag*)tag, f, "ID3v2 tag");
	    }
	    if ( (tag = tlf.APETag(false)) ) {
	        _displayAPETag((const TagLib::APE::Tag*)tag, f, "APE tag");
	    }
            continue;
	}

        // below are write ops
        if ( !(st.st_mode & (S_IWUSR | S_IWGRP) )) {
            MP3_TAG_WARN(f << " - permision denied, no updates made");
            continue;
        }



	TagLib::MPEG::File*  _tlf = new TagLib::MPEG::File(f, false);
	TagLib::MPEG::File&  tlf = *_tlf;


	if (i-1 == optind && enc == TagLib::String::Latin1) {
	    MP3_TAG_WARN("tagging as latin1, all multibyte chars will be incorrectly encoded!");
	}


	if (clean)
	{
	    /* take artist,title,album,yr,genre,track and get rid of the rest
	     */
	    IFlds  o;
	    MP3_TAG_NOTICE_VERBOSE("cleaning tags " << f);
	    if ( (tag = tlf.ID3v1Tag(false)) ) {
		o.populate(tag);

		tlf.strip(TagLib::MPEG::File::ID3v1);
		tag = tlf.ID3v1Tag(true);
		_tag(tag, o);
	    }

	    if ( (tag = tlf.ID3v2Tag(false)) ) {
		o.populate(tag);

		tlf.strip(TagLib::MPEG::File::ID3v2);

		/* tag is now gone, so re-create and repopulate
		 */
		tag = tlf.ID3v2Tag(true);
		_tag((TagLib::ID3v2::Tag*)tag, o, enc);
	    }
	}


	if (rmtag) {
	    MP3_TAG_DEBUG("stripping tags: " << rmtag);
	    MP3_TAG_NOTICE_VERBOSE("stripping tag " << f);
	    if ( !tlf.strip(rmtag)) {
		MP3_TAG_WARN_VERBOSE("'" << f << "' unable to strip tags " << rmtag);
	    }
	}
	else 
	{
	    if (mbconvert)
	    {
		TagLib::Tag *id2, *id1;
		if ( (id2 = tlf.ID3v2Tag(false)) == NULL &&
		     (id1 = tlf.ID3v1Tag(false)) == NULL) {
		    MP3_TAG_WARN_VERBOSE("no current tags to convert to multibyte '" << f << "'");
		}
		else
		{
		    tag = id2 ? id2 : id1;

		    static TagLib::String  a,t,A,g;
		    static char  y[5];
		    static char  T[3];

		    static TagLib::ByteVector  va, vt, vA, vg;

		    a = tag->artist();
		    t = tag->title();
		    A = tag->album();
		    g = tag->genre();

		    iflds.artist = (a == TagLib::String::null) ? NULL : (va = a.data(mbenc)).data();
		    iflds.title = (t == TagLib::String::null) ? NULL : (vt = t.data(mbenc)).data();
		    iflds.album = (A == TagLib::String::null) ? NULL : (vA = A.data(mbenc)).data();
		    iflds.genre = (g == TagLib::String::null) ? NULL : (vg = g.data(mbenc)).data();

		    sprintf(T, "%ld", tag->track());
		    sprintf(y, "%ld", tag->year());
		    iflds.trackno = T;
		    iflds.yr = y;
		}
	    }
	}

	if (svtag)
	{
	    if (svtag & TagLib::MPEG::File::ID3v1) {
		MP3_TAG_DEBUG("processing ID3v1");
		TagLib::ID3v1::Tag*  tag;
		if ( (tag = tlf.ID3v1Tag(false)) == NULL) {
		    MP3_TAG_DEBUG("adding ID3v1");
		    MP3_TAG_WARN_VERBOSE("no ID3v1 tag on " << f << ", creating..");
		    tag = tlf.ID3v1Tag(true);
		}
		_tag(tag, iflds);
	    }

	    if (svtag & TagLib::MPEG::File::ID3v2) {
		MP3_TAG_DEBUG("processing ID3v2");
		TagLib::ID3v2::Tag*  tag;
		if ( (tag = tlf.ID3v2Tag(false)) == NULL) {
		    MP3_TAG_DEBUG("adding ID3v2");
		    MP3_TAG_WARN_VERBOSE("no ID3v2 tag on " << f << ", creating..");
		    tag = tlf.ID3v2Tag(true);
		}
		_tag(tag, iflds, enc);
	    }


	    bool  saveok = tlf.save(svtag, false);
	    delete _tlf;
	    if (!saveok) {
		MP3_TAG_ERR("unable to save tag for '" << f << "'");
	    }
	    else
	    {
                if (prsvtm) {
                    /* try to reset the timestamps on the file
                     */
                    struct utimbuf  ub;
                    ub.actime = st.st_atime;
                    ub.modtime =  st.st_mtime;
                    if ( utime(f, &ub) < 0) {
                        MP3_TAG_WARN_VERBOSE("'" << f << "' unable to revert to original access times - " << strerror(errno));
                    }
                }
	    }
	}
#ifdef DEBUG
	{
	    /* looks like the tag() pulled out refs to a totally diff block of
	     * data off the file
	     */
	    f = argv[i-1];
	    MP3_TAG_DEBUG("opening " << f);
	    TagLib::MPEG::File  tlf(f, false);
	    TagLib::ID3v2::Tag*  tag;
	    if ( (tag = tlf.ID3v2Tag()) == NULL) {
		MP3_TAG_DEBUG("no ID3v2 tag on " << f);
	    }
	    else {
		_displayID3v2Tag(tag, f, "debug tag save");
	    }
	}
#endif
    }
    return 0;
}
