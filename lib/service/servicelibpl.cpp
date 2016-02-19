#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/base/init_num.h>
#include <lib/base/init.h>
#include <lib/base/nconfig.h>
#include <lib/base/object.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/decoder.h>
#include <lib/components/file_eraser.h>
#include <lib/gui/esubtitle.h>
#include <lib/service/servicelibpl.h>
#include <lib/service/service.h>
#include <lib/gdi/gpixmap.h>
#include <string>

void ep3Blit()
{
	fbClass *fb = fbClass::getInstance();
	fb->blit();
}

eServiceFactoryMP3::eServiceFactoryMP3()
{
	ePtr<eServiceCenter> sc;
	eServiceCenter::getPrivInstance(sc);
	if (sc)
	{
		std::list<std::string> extensions;
		extensions.push_back("dts");
		extensions.push_back("mp2");
		extensions.push_back("mp3");
		extensions.push_back("ogg");
		extensions.push_back("ogm");
		extensions.push_back("ogv");
		extensions.push_back("mpg");
		extensions.push_back("vob");
		extensions.push_back("wav");
		extensions.push_back("wave");
		extensions.push_back("m4v");
		extensions.push_back("mkv");
		extensions.push_back("avi");
		extensions.push_back("divx");
		extensions.push_back("dat");
		extensions.push_back("flac");
		extensions.push_back("flv");
		extensions.push_back("mp4");
		extensions.push_back("mov");
		extensions.push_back("m4a");
		extensions.push_back("3gp");
		extensions.push_back("3g2");
		extensions.push_back("asf");
		extensions.push_back("mpeg");
		extensions.push_back("m2ts");
		extensions.push_back("trp");
		extensions.push_back("vdr");
		extensions.push_back("mts");
		extensions.push_back("rar");
		extensions.push_back("img");
		extensions.push_back("iso");
		extensions.push_back("ifo");
		extensions.push_back("wmv");
		extensions.push_back("wma");
		sc->addServiceFactory(eServiceFactoryMP3::id, this, extensions);
	}

	m_service_info = new eStaticServiceMP3Info();
}

eServiceFactoryMP3::~eServiceFactoryMP3()
{
	ePtr<eServiceCenter> sc;
	eServiceCenter::getPrivInstance(sc);

	if (sc)
		sc->removeServiceFactory(eServiceFactoryMP3::id);
}

DEFINE_REF(eServiceFactoryMP3)

// iServiceHandler
RESULT eServiceFactoryMP3::play(const eServiceReference &ref, ePtr<iPlayableService> &ptr)
{
	// check resources...
	ptr = new eServiceMP3(ref);
	return 0;
}

RESULT eServiceFactoryMP3::record(const eServiceReference &ref, ePtr<iRecordableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryMP3::list(const eServiceReference &, ePtr<iListableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryMP3::info(const eServiceReference &ref, ePtr<iStaticServiceInformation> &ptr)
{
	ptr = m_service_info;
	return 0;
}

class eMP3ServiceOfflineOperations: public iServiceOfflineOperations
{
	DECLARE_REF(eMP3ServiceOfflineOperations);
	eServiceReference m_ref;
public:
	eMP3ServiceOfflineOperations(const eServiceReference &ref);
	RESULT deleteFromDisk(int simulate);
	RESULT getListOfFilenames(std::list<std::string> &);
	RESULT reindex();
};

DEFINE_REF(eMP3ServiceOfflineOperations);

eMP3ServiceOfflineOperations::eMP3ServiceOfflineOperations(const eServiceReference &ref): m_ref((const eServiceReference&)ref)
{
}

RESULT eMP3ServiceOfflineOperations::deleteFromDisk(int simulate)
{
	if (!simulate)
	{
		std::list<std::string> res;
		if (getListOfFilenames(res))
			return -1;
		eBackgroundFileEraser *eraser = eBackgroundFileEraser::getInstance();
		if (!eraser)
			eDebug("[eServiceMP3::%s] FATAL !! can't get background file eraser", __func__);
		for (std::list<std::string>::iterator i(res.begin()); i != res.end(); ++i)
		{
			eDebug("[eServiceMP3::%s] Removing %s...", __func__, i->c_str());
			if (eraser)
				eraser->erase(i->c_str());
			else
				::unlink(i->c_str());
		}
	}

	return 0;
}

RESULT eMP3ServiceOfflineOperations::getListOfFilenames(std::list<std::string> &res)
{
	res.clear();
	res.push_back(m_ref.path);
	return 0;
}

RESULT eMP3ServiceOfflineOperations::reindex()
{
	return -1;
}


RESULT eServiceFactoryMP3::offlineOperations(const eServiceReference &ref, ePtr<iServiceOfflineOperations> &ptr)
{
	ptr = new eMP3ServiceOfflineOperations(ref);
	return 0;
}

// eStaticServiceMP3Info


// eStaticServiceMP3Info is seperated from eServiceMP3 to give information
// about unopened files.

// probably eServiceMP3 should use this class as well, and eStaticServiceMP3Info
// should have a database backend where ID3-files etc. are cached.
// this would allow listing the mp3 database based on certain filters.

DEFINE_REF(eStaticServiceMP3Info)

eStaticServiceMP3Info::eStaticServiceMP3Info()
{
}

RESULT eStaticServiceMP3Info::getName(const eServiceReference &ref, std::string &name)
{
	if ( ref.name.length() )
		name = ref.name;
	else
	{
		size_t last = ref.path.rfind('/');
		if (last != std::string::npos)
			name = ref.path.substr(last+1);
		else
			name = ref.path;
	}

	return 0;
}

int eStaticServiceMP3Info::getLength(const eServiceReference &ref)
{
	return -1;
}

int eStaticServiceMP3Info::getInfo(const eServiceReference &ref, int w)
{
	switch (w)
	{
	case iServiceInformation::sTimeCreate:
		{
			struct stat s;
			if (stat(ref.path.c_str(), &s) == 0)
			{
				return s.st_mtime;
			}
		}
		break;
	case iServiceInformation::sFileSize:
		{
			struct stat s;
			if (stat(ref.path.c_str(), &s) == 0)
			{
				return s.st_size;
			}
		}
		break;
	}

	return iServiceInformation::resNA;
}

long long eStaticServiceMP3Info::getFileSize(const eServiceReference &ref)
{
	struct stat s;
	if (stat(ref.path.c_str(), &s) == 0)
	{
		return s.st_size;
	}

	return 0;
}

RESULT eStaticServiceMP3Info::getEvent(const eServiceReference &ref, ePtr<eServiceEvent> &evt, time_t start_time)
{
	if (ref.path.find("://") != std::string::npos)
	{
		eServiceReference equivalentref(ref);
		equivalentref.type = eServiceFactoryMP3::id;
		equivalentref.path.clear();
		return eEPGCache::getInstance()->lookupEventTime(equivalentref, start_time, evt);
	}

	evt = 0;
	return -1;
}

DEFINE_REF(eStreamBufferInfo)

eStreamBufferInfo::eStreamBufferInfo(int percentage, int inputrate, int outputrate, int space, int size)
: bufferPercentage(percentage),
	inputRate(inputrate),
	outputRate(outputrate),
	bufferSpace(space),
	bufferSize(size)
{
}

int eStreamBufferInfo::getBufferPercentage() const
{
	return bufferPercentage;
}

int eStreamBufferInfo::getAverageInputRate() const
{
	return inputRate;
}

int eStreamBufferInfo::getAverageOutputRate() const
{
	return outputRate;
}

int eStreamBufferInfo::getBufferSpace() const
{
	return bufferSpace;
}

int eStreamBufferInfo::getBufferSize() const
{
	return bufferSize;
}

eServiceMP3 *eServiceMP3::instance;

eServiceMP3 *eServiceMP3::getInstance()
{
	return instance;
}

eServiceMP3::eServiceMP3(eServiceReference ref):
	m_nownext_timer(eTimer::create(eApp)),
	m_cuesheet_changed(0),
	m_cutlist_enabled(1),
	m_ref(ref),
	m_pump(eApp, 1)
{
	eDebug("[eServiceMP3::%s]", __func__);
	m_currentAudioStream = -1;
	m_currentSubtitleStream = -1;
	m_cachedSubtitleStream = -1; /* report the first subtitle stream to be 'cached'. TODO: use an actual cache. */
	m_subtitle_widget = 0;
	m_buffer_size = 5 * 1024 * 1024;
	m_cuesheet_loaded = false; /* cuesheet CVR */
	inst_m_pump = &m_pump;
	CONNECT(m_nownext_timer->timeout, eServiceMP3::updateEpgCacheNowNext);
	CONNECT(inst_m_pump->recv_msg, eServiceMP3::gotThreadMessage);
	m_aspect = m_width = m_height = m_framerate = m_progressive = -1;
	m_state = stIdle;
	instance = this;

	player = (Context_t*) malloc(sizeof(Context_t));

	if (player)
	{
		player->playback  = &PlaybackHandler;
		player->output    = &OutputHandler;
		player->container = &ContainerHandler;
		player->manager   = &ManagerHandler;
		eDebug("[eServiceMP3::%s] %s", __func__, player->output->Name);
	}

	//Registration of output devices
	if (player && player->output)
	{
		player->output->Command(player,OUTPUT_ADD, (void*)"audio");
		player->output->Command(player,OUTPUT_ADD, (void*)"video");
		player->output->Command(player,OUTPUT_ADD, (void*)"subtitle");
	}

	if (player && player->output && player->output->subtitle)
	{
		fbClass *fb = fbClass::getInstance();
		SubtitleOutputDef_t out;
		out.screen_width = fb->getScreenResX();
		out.screen_height = fb->getScreenResY();
		out.shareFramebuffer = 1;
		out.framebufferFD = fb->getFD();
		out.destination = fb->getLFB_Direct();
		out.destStride = fb->Stride();
		out.framebufferBlit = ep3Blit;
		player->output->subtitle->Command(player, (OutputCmd_t)OUTPUT_SET_SUBTITLE_OUTPUT, (void*) &out);
	}

	//create playback path
	char file[1023] = {""};
	if ((!strncmp("http://", m_ref.path.c_str(), 7))
	|| (!strncmp("https://", m_ref.path.c_str(), 8))
	|| (!strncmp("cache://", m_ref.path.c_str(), 8))
	|| (!strncmp("concat://", m_ref.path.c_str(), 9))
	|| (!strncmp("crypto://", m_ref.path.c_str(), 9))
	|| (!strncmp("gopher://", m_ref.path.c_str(), 9))
	|| (!strncmp("hls://", m_ref.path.c_str(), 6))
	|| (!strncmp("hls+http://", m_ref.path.c_str(), 11))
	|| (!strncmp("httpproxy://", m_ref.path.c_str(), 12))
	|| (!strncmp("mmsh://", m_ref.path.c_str(), 7))
	|| (!strncmp("mmst://", m_ref.path.c_str(), 7))
	|| (!strncmp("rtmp://", m_ref.path.c_str(), 7))
	|| (!strncmp("rtmpe://", m_ref.path.c_str(), 8))
	|| (!strncmp("rtmpt://", m_ref.path.c_str(), 8))
	|| (!strncmp("rtmps://", m_ref.path.c_str(), 8))
	|| (!strncmp("rtmpte://", m_ref.path.c_str(), 9))
	|| (!strncmp("ftp://", m_ref.path.c_str(), 6))
	|| (!strncmp("rtp://", m_ref.path.c_str(), 6))
	|| (!strncmp("srtp://", m_ref.path.c_str(), 7))
	|| (!strncmp("subfile://", m_ref.path.c_str(), 10))
	|| (!strncmp("tcp://", m_ref.path.c_str(), 6))
	|| (!strncmp("tls://", m_ref.path.c_str(), 6))
	|| (!strncmp("udp://", m_ref.path.c_str(), 6))
	|| (!strncmp("udplite://", m_ref.path.c_str(), 10)))
		m_sourceinfo.is_streaming = true;
	else if ((!strncmp("file://", m_ref.path.c_str(), 7))
	|| (!strncmp("bluray://", m_ref.path.c_str(), 9))
	|| (!strncmp("hls+file://", m_ref.path.c_str(), 11))
	|| (!strncmp("myts://", m_ref.path.c_str(), 7)))
		;
	else
		strcat(file, "file://");

	strcat(file, m_ref.path.c_str());

	//try to open file
	if (player && player->playback && player->playback->Command(player, PLAYBACK_OPEN, file) >= 0)
	{
		//VIDEO
		//We dont have to register video tracks, or do we ?
		//AUDIO
		if (player && player->manager && player->manager->audio)
		{
			char ** TrackList = NULL;
			player->manager->audio->Command(player, MANAGER_LIST, &TrackList);
			if (TrackList != NULL)
			{
				eDebug("[eServiceMP3::%s] AudioTrack List:", __func__);
				int i = 0;
				for (i = 0; TrackList[i] != NULL; i+=2)
				{
					eDebug("[eServiceMP3::%s]\t%s - %s", __func__, TrackList[i], TrackList[i+1]);
					audioStream audio;
					audio.language_code = TrackList[i];

					// atUnknown, atMPEG, atMP3, atAC3, atDTS, atAAC, atPCM, atOGG, atFLAC
					if (    !strncmp("A_MPEG/L3",   TrackList[i+1], 9))
						audio.type = atMP3;
					else if (!strncmp("A_MP3",      TrackList[i+1], 5))
						audio.type = atMP3;
					else if (!strncmp("A_AC3",      TrackList[i+1], 5))
						audio.type = atAC3;
					else if (!strncmp("A_DTS",      TrackList[i+1], 5))
						audio.type = atDTS;
					else if (!strncmp("A_AAC",      TrackList[i+1], 5))
						audio.type = atAAC;
					else if (!strncmp("A_PCM",      TrackList[i+1], 5))
						audio.type = atPCM;
					else if (!strncmp("A_VORBIS",   TrackList[i+1], 8))
						audio.type = atOGG;
					else if (!strncmp("A_FLAC",     TrackList[i+1], 6))
						audio.type = atFLAC;
					else
						audio.type = atUnknown;

					m_audioStreams.push_back(audio);
					free(TrackList[i]);
					TrackList[i] = NULL;
					free(TrackList[i+1]);
					TrackList[i+1] = NULL;
				}
				free(TrackList);
				TrackList = NULL;
			}
		}

		//SUB
		if (player && player->manager && player->manager->subtitle)
		{
			char ** TrackList = NULL;
			player->manager->subtitle->Command(player, MANAGER_LIST, &TrackList);
			if (TrackList != NULL)
			{
				eDebug("[eServiceMP3::%s] SubtitleTrack List:", __func__);
				int i = 0;
				for (i = 0; TrackList[i] != NULL; i+=2)
				{
					eDebug("[eServiceMP3::%s]\t%s - %s", __func__, TrackList[i], TrackList[i+1]);
					subtitleStream sub;
					sub.language_code = TrackList[i];

					//  stPlainText, stSSA, stSRT
					if (     !strncmp("S_TEXT/SSA",   TrackList[i+1], 10) ||
							!strncmp("S_SSA", TrackList[i+1], 5))
						sub.type = stSSA;
					else if (!strncmp("S_TEXT/ASS",   TrackList[i+1], 10) ||
							!strncmp("S_AAS", TrackList[i+1], 5))
						sub.type = stSSA;
					else if (!strncmp("S_TEXT/SRT",   TrackList[i+1], 10) ||
							!strncmp("S_SRT", TrackList[i+1], 5))
						sub.type = stSRT;
					else
						sub.type = stPlainText;

					m_subtitleStreams.push_back(sub);
					free(TrackList[i]);
					TrackList[i] = NULL;
					free(TrackList[i+1]);
					TrackList[i+1] = NULL;
				}
				free(TrackList);
				TrackList = NULL;
			}
		}
		loadCuesheet(); /* cuesheet CVR */
		m_event(this, evStart);
	}
	else
	{
		//Creation failed, no playback support for insert file, so send e2 EOF to stop playback
		eDebug("[eServiceMP3::%s] ERROR! Creation failed! No playback support for insert file!", __func__);
		m_state = stRunning;
		m_event(this, evEOF);
	}
}

eServiceMP3::~eServiceMP3()
{
	if (m_subtitle_widget) m_subtitle_widget->destroy();
	m_subtitle_widget = 0;

	if (m_state == stRunning)
		stop();
}

void eServiceMP3::updateEpgCacheNowNext()
{
	bool update = false;
	ePtr<eServiceEvent> next = 0;
	ePtr<eServiceEvent> ptr = 0;
	eServiceReference ref(m_ref);
	ref.type = eServiceFactoryMP3::id;
	ref.path.clear();

	if (eEPGCache::getInstance() && eEPGCache::getInstance()->lookupEventTime(ref, -1, ptr) >= 0)
	{
		ePtr<eServiceEvent> current = m_event_now;
		if (!current || !ptr || current->getEventId() != ptr->getEventId())
		{
			update = true;
			m_event_now = ptr;
			time_t next_time = ptr->getBeginTime() + ptr->getDuration();
			if (eEPGCache::getInstance()->lookupEventTime(ref, next_time, ptr) >= 0)
			{
				next = ptr;
				m_event_next = ptr;
			}
		}
	}

	int refreshtime = 60;

	if (!next)
	{
		next = m_event_next;
	}

	if (next)
	{
		time_t now = eDVBLocalTimeHandler::getInstance()->nowTime();
		refreshtime = (int)(next->getBeginTime() - now) + 3;

		if (refreshtime <= 0 || refreshtime > 60)
		{
			refreshtime = 60;
		}
	}

	m_nownext_timer->startLongTimer(refreshtime);

	if (update)
	{
		m_event((iPlayableService*)this, evUpdatedEventInfo);
	}
}

DEFINE_REF(eServiceMP3);

RESULT eServiceMP3::connectEvent(const Slot2<void,iPlayableService*,int> &event, ePtr<eConnection> &connection)
{
	connection = new eConnection((iPlayableService*)this, m_event.connect(event));
	m_event(this, evSeekableStatusChanged);
	return 0;
}

RESULT eServiceMP3::start()
{
	if (m_state != stIdle)
	{
		eDebug("[eServiceMP3::%s] m_state != stIdle", __func__);
		return -1;
	}

	if (player && player->output && player->playback)
	{
		m_state = stRunning;

		player->output->Command(player, OUTPUT_OPEN, NULL);
		player->playback->Command(player, PLAYBACK_PLAY, NULL);
		m_event(this, evStart);
		m_event(this, evGstreamerPlayStarted);
		updateEpgCacheNowNext();
		eDebug("[eServiceMP3::%s] start %s", __func__, m_ref.path.c_str());

		return 0;
	}

	eDebug("[eServiceMP3::%s] ERROR in start %s", __func__, m_ref.path.c_str());
	return -1;
}

RESULT eServiceMP3::stop()
{
	if (m_state == stIdle)
	{
		eDebug("[eServiceMP3::%s] m_state == stIdle", __func__);
		return -1;
	}

	if (m_state == stStopped)
		return -1;

	eDebug("[eServiceMP3::%s] stop %s", __func__, m_ref.path.c_str());

	if (player && player->playback && player->output)
	{
		player->playback->Command(player, PLAYBACK_STOP, NULL);
		player->output->Command(player, OUTPUT_CLOSE, NULL);
	}

	if (player && player->output)
	{
		player->output->Command(player,OUTPUT_DEL, (void*)"audio");
		player->output->Command(player,OUTPUT_DEL, (void*)"video");
		player->output->Command(player,OUTPUT_DEL, (void*)"subtitle");
	}

	if (player && player->playback)
		player->playback->Command(player,PLAYBACK_CLOSE, NULL);

	if (player)
		free(player);

	if (player != NULL)
		player = NULL;

	m_state = stStopped;
	saveCuesheet();
	m_nownext_timer->stop();
	return 0;
}

RESULT eServiceMP3::setTarget(int target)
{
	return -1;
}

RESULT eServiceMP3::pause(ePtr<iPauseableService> &ptr)
{
	ptr=this;
	m_event((iPlayableService*)this, evUpdatedInfo);
	return 0;
}

int speed_mapping[] =
{
 /* e2_ratio   speed */
	2,         1,
	4,         3,
	8,         7,
	16,        15,
	32,        31,
	64,        63,
	128,      127,
	-2,       -5,
	-4,      -10,
	-8,      -20,
	-16,      -40,
	-32,      -80,
	-64,     -160,
	-128,     -320,
	-1,       -1
};

int getSpeed(int ratio)
{
	int i = 0;
	while (speed_mapping[i] != -1)
	{
		if (speed_mapping[i] == ratio)
			return speed_mapping[i+1];
		i += 2;
	}

	return -1;
}

RESULT eServiceMP3::setSlowMotion(int ratio)
{
	// konfetti: in libeplayer3 we changed this because I dont like application specific stuff in a library
	int speed = getSpeed(ratio);

	if (player && player->playback && (speed != -1))
	{
		int result = 0;

		if (ratio > 1)
			result = player->playback->Command(player, PLAYBACK_SLOWMOTION, (void*)&speed);

		if (result != 0)
			return -1;
	}

	return 0;
}

RESULT eServiceMP3::setFastForward(int ratio)
{
	// konfetti: in libeplayer3 we changed this because I dont like application specific stuff in a library
	int speed = getSpeed(ratio);

	if (player && player->playback && (speed != -1))
	{
		int result = 0;

		if (ratio > 1)
			result = player->playback->Command(player, PLAYBACK_FASTFORWARD, (void*)&speed);
		else if (ratio < -1)
		{
			//speed = speed * -1;
			result = player->playback->Command(player, PLAYBACK_FASTBACKWARD, (void*)&speed);
		}
		else
			result = player->playback->Command(player, PLAYBACK_CONTINUE, NULL);

		if (result != 0)
			return -1;
	}

	return 0;
}

		// iPausableService
RESULT eServiceMP3::pause()
{
	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_PAUSE, NULL);

	return 0;
}

RESULT eServiceMP3::unpause()
{
	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_CONTINUE, NULL);

	return 0;
}

	/* iSeekableService */
RESULT eServiceMP3::seek(ePtr<iSeekableService> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::getLength(pts_t &pts)
{
	double length = 0;

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_LENGTH, &length);

	if (length <= 0)
		return -1;

	pts = length * 90000;
	return 0;
}

RESULT eServiceMP3::seekTo(pts_t to)
{
	float pos = (to/90000.0)-10;

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_SEEK, (void*)&pos);

	return 0;
}

RESULT eServiceMP3::seekRelative(int direction, pts_t to)
{
	float pos = direction*(to/90000.0);

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_SEEK, (void*)&pos);

	return 0;
}

RESULT eServiceMP3::getPlayPosition(pts_t &pts)
{
	if (player && player->playback && !player->playback->isPlaying)
	{
		eDebug("[eServiceMP3::%s] !!!!EOF!!!!", __func__);

		if(m_state == stRunning)
			m_event((iPlayableService*)this, evEOF);

		pts = 0;
		return -1;
	}

	unsigned long long int vpts = 0;

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_PTS, &vpts);

	if (vpts<=0)
		return -1;

	/* len is in nanoseconds. we have 90 000 pts per second. */
	pts = vpts;
	return 0;
}

RESULT eServiceMP3::setTrickmode(int trick)
{
	/* trickmode is not yet supported by our dvbmediasinks. */
	return -1;
}

RESULT eServiceMP3::isCurrentlySeekable()
{
	// Hellmaster1024: 1 for skipping 3 for skipping anf fast forward
	return 3;
}

RESULT eServiceMP3::info(ePtr<iServiceInformation>&i)
{
	i = this;
	return 0;
}

RESULT eServiceMP3::getName(std::string &name)
{
	std::string title = m_ref.getName();

	if (title.empty())
	{
		name = m_ref.path;
		size_t n = name.rfind('/');
		if (n != std::string::npos)
			name = name.substr(n + 1);
	}
	else
		name = title;

	return 0;
}

RESULT eServiceMP3::getEvent(ePtr<eServiceEvent> &evt, int nownext)
{
	evt = nownext ? m_event_next : m_event_now;
	if (!evt)
		return -1;

	return 0;
}

int eServiceMP3::getInfo(int w)
{
	switch (w)
	{
	case sServiceref: return m_ref;
	case sVideoHeight: return m_height;
	case sVideoWidth: return m_width;
	case sFrameRate: return m_framerate;
	case sProgressive: return m_progressive;
	case sAspect: return m_aspect;
	case sTagTitle:
	case sTagArtist:
	case sTagAlbum:
	case sTagTitleSortname:
	case sTagArtistSortname:
	case sTagAlbumSortname:
	case sTagDate:
	case sTagComposer:
	case sTagGenre:
	case sTagComment:
	case sTagExtendedComment:
	case sTagLocation:
	case sTagHomepage:
	case sTagDescription:
	case sTagVersion:
	case sTagISRC:
	case sTagOrganization:
	case sTagCopyright:
	case sTagCopyrightURI:
	case sTagContact:
	case sTagLicense:
	case sTagLicenseURI:
	case sTagCodec:
	case sTagAudioCodec:
	case sTagVideoCodec:
	case sTagEncoder:
	case sTagLanguageCode:
	case sTagKeywords:
	case sTagChannelMode:
	case sUser+12:
		return resIsString;
	case sTagTrackGain:
	case sTagTrackPeak:
	case sTagAlbumGain:
	case sTagAlbumPeak:
	case sTagReferenceLevel:
	case sTagBeatsPerMinute:
	case sTagImage:
	case sTagPreviewImage:
	case sTagAttachment:
		return resIsPyObject;
	case sBuffer: return m_bufferInfo.bufferPercent;
	default:
		return resNA;
	}

	return 0;
}

std::string eServiceMP3::getInfoString(int w)
{
	if ( m_sourceinfo.is_streaming )
	{
		switch (w)
		{
		case sProvider:
			return "IPTV";
		case sServiceref:
		{
			eServiceReference ref(m_ref);
			ref.type = eServiceFactoryMP3::id;
			ref.path.clear();
			return ref.toString();
		}
		default:
			break;
		}
	}

	char * tag = NULL;
	switch (w)
	{
	case sTagTitle:
		tag = strdup("Title");
		break;
	case sTagArtist:
		tag = strdup("Artist");
		break;
	case sTagAlbum:
		tag = strdup("Album");
		break;
	case sTagComment:
		tag = strdup("Comment");
		break;
	case sTagTrackNumber:
		tag = strdup("Track");
		break;
	case sTagGenre:
		tag = strdup("Genre");
		break;
	case sTagDate:
		tag = strdup("Year");
		break;
	case sTagVideoCodec:
		tag = strdup("VideoType");
		break;
	case sTagAudioCodec:
		tag = strdup("AudioType");
		break;
	default:
		return "";
	}

	if (player && player->playback)
	{
		if (player->playback->Command(player, PLAYBACK_INFO, &tag) == 0)
		{
			std::string res (tag);
			free(tag);
			return res;
		}
	}

	return "";
}

RESULT eServiceMP3::audioChannel(ePtr<iAudioChannelSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::audioTracks(ePtr<iAudioTrackSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::cueSheet(ePtr<iCueSheet> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::subtitle(ePtr<iSubtitleOutput> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::audioDelay(ePtr<iAudioDelay> &ptr)
{
	ptr = this;
	return 0;
}

int eServiceMP3::getNumberOfTracks()
{
 	return m_audioStreams.size();
}

int eServiceMP3::getCurrentTrack()
{
	return m_currentAudioStream;
}

RESULT eServiceMP3::selectTrack(unsigned int i)
{
	int ret = selectAudioStream(i);
	return ret;
}

int eServiceMP3::selectAudioStream(int i)
{
	if (i != m_currentAudioStream)
	{
		if (player && player->playback)
			player->playback->Command(player, PLAYBACK_SWITCH_AUDIO, (void*)&i);
		m_currentAudioStream=i;
		return 0;
	}

	return -1;
}

int eServiceMP3::getCurrentChannel()
{
	return STEREO;
}

RESULT eServiceMP3::selectChannel(int i)
{
	eDebug("[eServiceMP3::%s] %i", __func__,i);
	return 0;
}

RESULT eServiceMP3::getTrackInfo(struct iAudioTrackInfo &info, unsigned int i)
{
 	if (i >= m_audioStreams.size())
		return -2;
	if (m_audioStreams[i].type == atMPEG)
		info.m_description = "MPEG";
	else if (m_audioStreams[i].type == atMP3)
		info.m_description = "MP3";
	else if (m_audioStreams[i].type == atAC3)
		info.m_description = "AC3";
	else if (m_audioStreams[i].type == atAAC)
		info.m_description = "AAC";
	else if (m_audioStreams[i].type == atDTS)
		info.m_description = "DTS";
	else if (m_audioStreams[i].type == atPCM)
		info.m_description = "PCM";
	else if (m_audioStreams[i].type == atOGG)
		info.m_description = "OGG";
	if (info.m_language.empty())
		info.m_language = m_audioStreams[i].language_code;

	return 0;
}

eAutoInitPtr<eServiceFactoryMP3> init_eServiceFactoryMP3(eAutoInitNumbers::service+1, "eServiceFactoryMP3");

RESULT eServiceMP3::enableSubtitles(iSubtitleUser *user, struct SubtitleTrack &track)
{
	if (m_currentSubtitleStream != track.pid)
	{
		m_subtitle_pages.clear();
		m_currentSubtitleStream = track.pid;
		m_cachedSubtitleStream = m_currentSubtitleStream;
		m_subtitle_widget = user;
		
		eDebug ("eServiceMP3::switched to subtitle stream %i", m_currentSubtitleStream);

		if (player && player->playback)
			player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&track.pid);

		// we have to force a seek, before the new subtitle stream will start
		seekRelative(-1, 90000);
	}

	return 0;
}

RESULT eServiceMP3::disableSubtitles()
{
	eDebug("[eServiceMP3::%s]", __func__);
	m_currentSubtitleStream = -1;
	m_cachedSubtitleStream = m_currentSubtitleStream;
	m_subtitle_pages.clear();

	if (m_subtitle_widget) m_subtitle_widget->destroy();

	m_subtitle_widget = 0;

	int pid = -1;

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&pid);

	return 0;
}

RESULT eServiceMP3::getCachedSubtitle(struct SubtitleTrack &track)
{

	bool autoturnon = eConfigManager::getConfigBoolValue("config.subtitles.pango_autoturnon", true);

	if (!autoturnon)
		return -1;

	if (m_cachedSubtitleStream >= 0 && m_cachedSubtitleStream < (int)m_subtitleStreams.size())
	{
		track.type = 2;
		track.pid = m_cachedSubtitleStream;
		track.page_number = int(m_subtitleStreams[m_cachedSubtitleStream].type);
		track.magazine_number = 0;
		return 0;
	}

	return -1;
}

RESULT eServiceMP3::getSubtitleList(std::vector<struct SubtitleTrack> &subtitlelist)
{
	// 	eDebug("[eServiceMP3::%s]", __func__);
	int stream_idx = 0;

	for (std::vector<subtitleStream>::iterator IterSubtitleStream(m_subtitleStreams.begin()); IterSubtitleStream != m_subtitleStreams.end(); ++IterSubtitleStream)
	{
		subtype_t type = IterSubtitleStream->type;
		switch(type)
		{
		case stUnknown:
		case stVOB:
		case stPGS:
			break;
		default:
		{
			struct SubtitleTrack track;
			track.type = 2;
			track.pid = stream_idx;
			track.page_number = int(type);
			track.magazine_number = 0;
			track.language_code = IterSubtitleStream->language_code;
			subtitlelist.push_back(track);
		}
		}
		stream_idx++;
	}

	eDebug("[eServiceMP3::%s] finished", __func__);
	return 0;
}

RESULT eServiceMP3::streamed(ePtr<iStreamedService> &ptr)
{
	ptr = this;
	return 0;
}

ePtr<iStreamBufferInfo> eServiceMP3::getBufferCharge()
{
	return new eStreamBufferInfo(m_bufferInfo.bufferPercent, m_bufferInfo.avgInRate, m_bufferInfo.avgOutRate, m_bufferInfo.bufferingLeft, m_buffer_size);
}

/* cuesheet CVR */
PyObject *eServiceMP3::getCutList()
{
	ePyObject list = PyList_New(0);

	for (std::multiset<struct cueEntry>::iterator i(m_cue_entries.begin()); i != m_cue_entries.end(); ++i)
	{
		ePyObject tuple = PyTuple_New(2);
		PyTuple_SET_ITEM(tuple, 0, PyLong_FromLongLong(i->where));
		PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(i->what));
		PyList_Append(list, tuple);
		Py_DECREF(tuple);
	}

	return list;
}

/* cuesheet CVR */
void eServiceMP3::setCutList(ePyObject list)
{
	if (!PyList_Check(list))
		return;
	int size = PyList_Size(list);
	int i;

	m_cue_entries.clear();

	for (i=0; i<size; ++i)
	{
		ePyObject tuple = PyList_GET_ITEM(list, i);
		if (!PyTuple_Check(tuple))
		{
			eDebug("[eServiceMP3::%s] non-tuple in cutlist", __func__);
			continue;
		}
		if (PyTuple_Size(tuple) != 2)
		{
			eDebug("[eServiceMP3::%s] cutlist entries need to be a 2-tuple", __func__);
			continue;
		}
		ePyObject ppts = PyTuple_GET_ITEM(tuple, 0), ptype = PyTuple_GET_ITEM(tuple, 1);
		if (!(PyLong_Check(ppts) && PyInt_Check(ptype)))
		{
			eDebug("[eServiceMP3::%s] cutlist entries need to be (pts, type)-tuples (%d %d)", __func__, PyLong_Check(ppts), PyInt_Check(ptype));
			continue;
		}
		pts_t pts = PyLong_AsLongLong(ppts);
		int type = PyInt_AsLong(ptype);
		m_cue_entries.insert(cueEntry(pts, type));
		eDebug("[eServiceMP3::%s] adding %08llx, %d", __func__, pts, type);
	}
	m_cuesheet_changed = 1;
	m_event((iPlayableService*)this, evCuesheetChanged);
}

void eServiceMP3::setCutListEnable(int enable)
{
	m_cutlist_enabled = enable;
}

int eServiceMP3::setBufferSize(int size)
{
	m_buffer_size = size;
	return 0;
}

int eServiceMP3::getAC3Delay()
{
	return 0;
}

int eServiceMP3::getPCMDelay()
{
	return 0;
}

void eServiceMP3::setAC3Delay(int delay)
{
}

void eServiceMP3::setPCMDelay(int delay)
{
}

void eServiceMP3::gotThreadMessage(const int &msg)
{
	switch(msg)
	{
	case 1: // thread stopped
		eDebug("[eServiceMP3::%s] issuing eof...", __func__);
		m_event(this, evEOF);
		break;
	}
}

/* cuesheet CVR */
void eServiceMP3::loadCuesheet()
{
	if (!m_cuesheet_loaded)
	{
		eDebug("[eServiceMP3::%s] loading cuesheet", __func__);
		m_cuesheet_loaded = true;
	}

	std::string filename = m_ref.path + ".cuts";

	m_cue_entries.clear();

	FILE *f = fopen(filename.c_str(), "rb");

	if (f)
	{
		while (1)
		{
			unsigned long long where;
			unsigned int what;

			if (!fread(&where, sizeof(where), 1, f))
				break;
			if (!fread(&what, sizeof(what), 1, f))
				break;

			where = be64toh(where);
			what = ntohl(what);

			if (what > 3)
				break;

			m_cue_entries.insert(cueEntry(where, what));
		}
		fclose(f);
		eDebug("[eServiceMP3::%s] cuts file has %zd entries", __func__, m_cue_entries.size());
	} else
		eDebug("[eServiceMP3::%s] cutfile not found!", __func__);

	m_cuesheet_changed = 0;
	m_event((iPlayableService*)this, evCuesheetChanged);
}

/* cuesheet CVR */
void eServiceMP3::saveCuesheet()
{
	std::string filename = m_ref.path;

	/* save cuesheet only when main file is accessible. */
	if (::access(filename.c_str(), R_OK) < 0)
		return;

	filename.append(".cuts");
	/* do not save to file if there are no cuts */
	/* remove the cuts file if cue is empty */
	if(m_cue_entries.begin() == m_cue_entries.end())
	{
		if (::access(filename.c_str(), F_OK) == 0)
			remove(filename.c_str());
		return;
	}

	FILE *f = fopen(filename.c_str(), "wb");

	if (f)
	{
		unsigned long long where;
		int what;

		for (std::multiset<cueEntry>::iterator i(m_cue_entries.begin()); i != m_cue_entries.end(); ++i)
		{
			where = htobe64(i->where);
			what = htonl(i->what);
			fwrite(&where, sizeof(where), 1, f);
			fwrite(&what, sizeof(what), 1, f);

		}
		fclose(f);
	}
	m_cuesheet_changed = 0;
}

void libeplayerThreadStop() // call from libeplayer
{
	eDebug("[eServiceMP3::%s]", __func__);
	eServiceMP3 *serv = eServiceMP3::getInstance();
	serv->inst_m_pump->send(1);
}

