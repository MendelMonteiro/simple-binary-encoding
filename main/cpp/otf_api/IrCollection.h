/*
 * Copyright 2013 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _IR_COLLECTION_H_
#define _IR_COLLECTION_H_

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>

#include <map>
#include <iostream>

#include "otf_api/Ir.h"
#include "uk_co_real_logic_sbe_ir_generated/SerializedToken.hpp"
#include "uk_co_real_logic_sbe_ir_generated/SerializedFrame.hpp"

using namespace sbe::on_the_fly;
using namespace uk_co_real_logic_sbe_ir_generated;

namespace sbe {
namespace on_the_fly {

class IrCollection
{
public:
    IrCollection() : header_(NULL)
    {
    };

    virtual ~IrCollection()
    {
        delete[] buffer_;
    };

    int loadFromFile(const char *filename)
    {
        if ((length_ = IrCollection::getFileSize(filename)) < 0)
        {
            return -1;
        }
        std::cout << "IR Filename " << filename << " length " << length_ << std::endl;
        buffer_ = new char[length_];

        if (IrCollection::readFileIntoBuffer(buffer_, filename, length_) < 0)
        {
            return -1;
        }

        if (processIr() < 0)
        {
            return -1;
        }
        return 0;
    };

    Ir &header(void) const
    {
        return *header_;
    };

    const Ir *message(int id) const
    {
        return map_.find(id)->second;
    };

protected:
    // OS specifics
    static int getFileSize(const char *filename)
    {
        struct stat fileStat;

        if (::stat(filename, &fileStat) != 0)
        {
            return -1;
        }

        return fileStat.st_size;
    };

    static int readFileIntoBuffer(char *buffer, const char *filename, int length)
    {
        FILE *fptr = ::fopen(filename, "r");
        int remaining = length;

        if (fptr == NULL)
        {
            return -1;
        }

        int fd = fileno(fptr);
        while (remaining > 0)
        {
            int sz = ::read(fd, buffer + (length - remaining), 4098);
            remaining -= sz;
            if (sz < 0)
            {
                break;
            }
        }
        fclose(fptr);
        return (remaining == 0) ? 0 : -1;
    };

    int processIr(void)
    {
        SerializedFrame frame;
        int offset = 0, tmpLen = 0;
        char tmp[256];

        frame.reset(buffer_, offset);
        tmpLen = frame.getPackageVal(tmp, sizeof(tmp));

        ::std::cout << "Reading IR package=\"" << std::string(tmp, tmpLen) << "\"" << ::std::endl;

        offset += frame.size();

        headerLength_ = readHeader(offset);

        offset += headerLength_;

        while (offset < length_)
        {
            offset += readMessage(offset);
        }

        return 0;
    };

    int readHeader(int offset)
    {
        SerializedToken token;
        int size = 0;

        while (offset + size < length_)
        {
            char tmp[256], name[256];
            int nameLen = 0;

            token.reset(buffer_, offset + size);

            nameLen = token.getName(name, sizeof(name));
            token.getConstVal(tmp, sizeof(tmp));
            token.getMinVal(tmp, sizeof(tmp));
            token.getMaxVal(tmp, sizeof(tmp));
            token.getNullVal(tmp, sizeof(tmp));
            token.getCharacterEncoding(tmp, sizeof(tmp));

            size += token.size();

            if (token.signal() == SerializedSignal::BEGIN_COMPOSITE)
            {
                std::cout << " Header name=\"" << std::string(name, nameLen) << "\"";
            }

            if (token.signal() == SerializedSignal::END_COMPOSITE)
            {
                break;
            }
        }

        std::cout << " length " << size << std::endl;

        header_ = new Ir(buffer_ + offset, size);

        return size;
    };

    int readMessage(int offset)
    {
        SerializedToken token;
        int size = 0;

        while (offset + size < length_)
        {
            char tmp[256], name[256];
            int nameLen = 0;

            token.reset(buffer_, offset + size);

            nameLen = token.getName(name, sizeof(name));
            token.getConstVal(tmp, sizeof(tmp));
            token.getMinVal(tmp, sizeof(tmp));
            token.getMaxVal(tmp, sizeof(tmp));
            token.getNullVal(tmp, sizeof(tmp));
            token.getCharacterEncoding(tmp, sizeof(tmp));

            size += token.size();

            if (token.signal() == SerializedSignal::BEGIN_MESSAGE)
            {
                std::cout << " Message name=\"" << std::string(name, nameLen) << "\"";
                std::cout << " id=\"" << token.schemaID() << "\"";
                std::cout << " version=\"" << token.tokenVersion() << "\"";
            }

            if (token.signal() == SerializedSignal::END_MESSAGE)
            {
                break;
            }
        }

        std::cout << " length " << size << std::endl;

        // save buffer_ + offset as start of message and size as length

        map_[token.schemaID()] = new Ir(buffer_ + offset, size);

        return size;
    };

private:
    std::map<int, Ir *> map_;

    char *buffer_;
    int length_;

    Ir *header_;
    int headerLength_;
};

} // namespace on_the_fly

} // namespace sbe

#endif /* _IR_COLLECTION_H_ */