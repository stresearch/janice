#include <janice_io_opencv.h>

#include <opencv2/highgui.hpp>

#include <string>
#include <vector>

namespace
{

static inline JaniceError cv_mat_to_janice_image(cv::Mat& m, JaniceImage* _image)
{
    // Allocate a new image
    JaniceImage image = new JaniceImageType();

    // Set up the dimensions
    image->channels = m.channels();
    image->rows = m.rows;
    image->cols = m.cols;

    image->data = (uint8_t*) malloc(m.channels() * m.rows * m.cols);
    memcpy(image->data, m.data, m.channels() * m.rows * m.cols);
    image->owner = true;

    *_image = image;

    return JANICE_SUCCESS;
}

// ----------------------------------------------------------------------------
// JaniceMediaIterator

struct JaniceMediaIteratorStateType
{
    std::vector<std::string> filenames;
    std::vector<uint32_t> frames;

    size_t pos;
};

JaniceError is_video(JaniceMediaIterator it, bool* video)
{
    *video = true; // Treat this as a video
    return JANICE_SUCCESS;
}

JaniceError get_frame_rate(JaniceMediaIterator, float*)
{
    return JANICE_INVALID_MEDIA;
}

JaniceError next(JaniceMediaIterator it, JaniceImage* image)
{
    JaniceMediaIteratorStateType* state = (JaniceMediaIteratorStateType*) it->_internal;

    if (state->pos == state->filenames.size())
        return JANICE_MEDIA_AT_END;

    try {
        cv::Mat cv_img = cv::imread(state->filenames[state->pos], cv::IMREAD_ANYCOLOR);
        cv_mat_to_janice_image(cv_img, image);
    } catch (...) {
        return JANICE_UNKNOWN_ERROR;
    }

    ++state->pos;

    return JANICE_SUCCESS;
}

JaniceError seek(JaniceMediaIterator, uint32_t)
{
    return JANICE_INVALID_MEDIA;
}

JaniceError get(JaniceMediaIterator, JaniceImage*, uint32_t)
{
    return JANICE_INVALID_MEDIA;
}

JaniceError tell(JaniceMediaIterator it, uint32_t* frame)
{
    JaniceMediaIteratorStateType* state = (JaniceMediaIteratorStateType*) it->_internal;

    if (state->pos == state->filenames.size())
        return JANICE_MEDIA_AT_END;

    *frame = state->frames[state->pos];

    return JANICE_SUCCESS;
}

JaniceError free_image(JaniceImage* image)
{
    if (image && (*image)->owner)
        free((*image)->data);
    delete (*image);

    return JANICE_SUCCESS;
}

JaniceError free_iterator(JaniceMediaIterator* it)
{
    if (it && (*it)->_internal) {
        delete (JaniceMediaIteratorStateType*) (*it)->_internal;
        delete (*it);
        *it = nullptr;
    }

    return JANICE_SUCCESS;
}

JaniceError reset(JaniceMediaIterator it)
{
    JaniceMediaIteratorStateType* state = (JaniceMediaIteratorStateType*) it->_internal;
    state->pos = 0;

    return JANICE_SUCCESS;
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// OpenCV I/O only, create a sparse opencv_io media iterator

JaniceError janice_io_opencv_create_sparse_media_iterator(const char** filenames, uint32_t* frames, size_t num_files, JaniceMediaIterator *_it)
{
    JaniceMediaIterator it = new JaniceMediaIteratorType();

    it->is_video = &is_video;
    it->get_frame_rate =  &get_frame_rate;

    it->next = &next;
    it->seek = &seek;
    it->get  = &get;
    it->tell = &tell;

    it->free_image = &free_image;
    it->free       = &free_iterator;

    it->reset      = &reset;

    JaniceMediaIteratorStateType* state = new JaniceMediaIteratorStateType();
    for (size_t i = 0; i < num_files; ++i) {
        state->filenames.push_back(std::string(filenames[i]));
        state->frames.push_back(frames[i]);
    }
    state->pos = 0;

    it->_internal = (void*) (state);

    *_it = it;

    return JANICE_SUCCESS;
}
