#ifndef EXTENDED_EXTRACTOR_
#define EXTENDED_EXTRACTOR_

namespace android {

class DataSource;
class MediaExtractor;
class String8;

class ExtendedExtractor
{
public:
    static sp<MediaExtractor> CreateExtractor(const sp<DataSource> &source, const char* mime);
    static void RegisterSniffers();
};

}  // namespace android

#endif //EXTENDED_EXTRACTOR_
