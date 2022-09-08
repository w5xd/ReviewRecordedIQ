/* Copyright (c) 2022, Wayne Wright, W5XD. All rights reserved. */
#pragma once
#include <fstream>
#include <functional>
#include <vector>
#include <cstring>
class RiffReader {
public:
    typedef std::function<void(const char *, unsigned, std::ifstream &)> RiffChunkFcn_t;
    typedef std::function<bool(unsigned char *, unsigned)> DataChunkFcn_t;
    typedef std::function<bool()> AtEndFcn_t;

    RiffReader(std::ifstream& instream)
        : format(0)
        , numChannels(0)
        , sampleRate(0)
        , byteRate(0)
        , blockAlign(0)
        , bitsPerSample(0)
        , dataChunkSize(0)
        , inputFile(instream)
    { }

    void ParseHeader()
    {
        std::vector<char> buf(4);
        inputFile.read(&buf[0], buf.size());
        if (strncmp(&buf[0], "RIFF", buf.size()) != 0)
            throw std::runtime_error("Input file missing RIFF header");
        uint32_t chunksize(0);
        inputFile.read(&buf[0], buf.size()); // entire file size
        for (int i = 3; i >= 0; i -= 1)
        {
            chunksize <<= 8;
            chunksize |= static_cast<unsigned char>(buf[i]);
        }
        inputFile.read(&buf[0], buf.size());
        if (strncmp(&buf[0], "WAVE", buf.size()) != 0)
            throw std::runtime_error("Input file missing WAVE header");
        inputFile.read(&buf[0], buf.size());
        if (strncmp(&buf[0], "fmt ", buf.size()) != 0)
            throw std::runtime_error("Input file missing fmt header");
        chunksize = 0;
        inputFile.read(&buf[0], buf.size()); // size of fmt
        for (int i = 3; i >= 0; i -= 1)
        {
            chunksize <<= 8;
            chunksize |= static_cast<unsigned char>(buf[i]);
        }

        inputFile.read(&buf[0], 2);
        format = (static_cast<unsigned char>(buf[1]) << 8) | static_cast<unsigned char>(buf[0]);

        inputFile.read(&buf[0], 2);
        numChannels = (static_cast<unsigned char>(buf[1]) << 8) | static_cast<unsigned char>(buf[0]);

        inputFile.read(&buf[0], buf.size());
        for (int i = 3; i >= 0; i -= 1)
        {
            sampleRate <<= 8;
            sampleRate |= static_cast<unsigned char>(buf[i]);
        }

        inputFile.read(&buf[0], buf.size());
        for (int i = 3; i >= 0; i -= 1)
        {
            byteRate <<= 8;
            byteRate |= static_cast<unsigned char>(buf[i]);
        }

        inputFile.read(&buf[0], 2);
        blockAlign = (static_cast<unsigned char>(buf[1]) << 8) | static_cast<unsigned char>(buf[0]);

        inputFile.read(&buf[0], 2);
        bitsPerSample = (static_cast<unsigned char>(buf[1]) << 8) | static_cast<unsigned char>(buf[0]);

        chunksize -= 16;
        while (chunksize-- > 0)
            inputFile.read(&buf[0], 1);
    }
    
    void ProcessChunks(const DataChunkFcn_t& dataFcn, const RiffChunkFcn_t &chunkFcn = RiffChunkFcn_t(),
            const AtEndFcn_t &atEnd = AtEndFcn_t())
    {
        std::vector<char> buf(4);
        std::vector<char> chunkTag(4);
        for (; !inputFile.eof();)
        {
            bool skip = false;
            inputFile.read(&chunkTag[0], chunkTag.size());
            if (inputFile.eof())
                break;
            if (strncmp(&chunkTag[0], "data", chunkTag.size()) != 0)
                skip = true;
            uint32_t chunksize(0);
            inputFile.read(&buf[0], buf.size()); // entire file size
            for (int i = 3; i >= 0; i -= 1)
            {
                chunksize <<= 8;
                chunksize |= static_cast<unsigned char>(buf[i]);
            }
            if (skip)
            {
                auto toSkip = inputFile.tellg();
                toSkip += chunksize;
                if (chunkFcn)
                    chunkFcn(&chunkTag[0], chunksize, inputFile);
                inputFile.seekg(toSkip);
                continue;
            }
            auto here = inputFile.tellg();
            dataChunkBegin = here;
            if (chunksize == 0) // reading an incompletely written file
            {
                // read to end of file.
                inputFile.seekg(0, inputFile.end);
                chunksize = static_cast<uint32_t>(inputFile.tellg() - here);
                inputFile.seekg(here);
            }
            dataChunkSize = chunksize;
            std::vector<unsigned char> chunkBuffer(blockAlign * 100);
            for (;;)
            {
                while (!inputFile.eof())
                {
                    inputFile.read(reinterpret_cast<char*>(&chunkBuffer[0]), chunkBuffer.size());
                    auto chunkBufferSize = inputFile.gcount();
                    if (chunkBufferSize == 0)
                        break;
                    unsigned char* p = &chunkBuffer[0];
                    unsigned numFrames = static_cast<unsigned>( chunkBufferSize / blockAlign);
                    if (dataFcn && !dataFcn(p, numFrames))
                        break;
                }
                if (!atEnd || atEnd())
                    break;
            }
            return;
        }
    }
 
    unsigned CurrentFrameNumber() const 
    {   // only valid after reading 'data'
        if (dataChunkSize == 0 || blockAlign == 0)
            return 0;
        if (!inputFile.eof())
            return static_cast<unsigned>(inputFile.tellg() - dataChunkBegin) / blockAlign;
        return dataChunkSize / blockAlign;
    }

    void SeekToFrameNumber(unsigned frame)
    {
        if (dataChunkSize != 0)
        {   // can only seek if we have made it to the beginning of 'data'
            auto pos = dataChunkBegin;
            pos += frame * blockAlign;
            auto end = dataChunkBegin;
            end += dataChunkSize;
            if (inputFile.eof())
                inputFile.clear();
            if (pos > 0 && pos <= end)
                inputFile.seekg(pos);
        }
    }
    
    uint16_t get_format() const {        return format;    }
    uint16_t get_numChannels() const { return numChannels;}
    uint32_t get_sampleRate() const { return sampleRate;}
    uint32_t get_byteRate() const { return byteRate;}
    uint16_t get_blockAlign() const { return blockAlign;}
    uint16_t get_bitsPerSample() const { return bitsPerSample;}
    uint32_t get_dataChunkSize() const { return dataChunkSize;}

protected:
    std::ifstream &inputFile;
    std::streampos dataChunkBegin;
    uint16_t format;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t dataChunkSize;

};
