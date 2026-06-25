#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>
#include "../Utils/CRC32_64/CRC32_64.hpp"

class FileBuffer {
public:
    std::vector<uint8_t> stream;

    void writeBytes(const void* data, size_t size) {
        const auto* bytePtr = static_cast<const uint8_t*>(data);
        stream.insert(stream.end(), bytePtr, bytePtr + size);
    }

    void writeString(const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.size());
        writeBytes(&len, sizeof(uint32_t));
        writeBytes(str.data(), len);
    }

    template<typename T>
    void writeVector(const std::vector<T>& vec) {
        uint32_t count = static_cast<uint32_t>(vec.size());
        writeBytes(&count, sizeof(uint32_t));
        if (count > 0) {
            writeBytes(vec.data(), count * sizeof(T));
        }
    }
};

class FileSystem {
private:
    static constexpr uint32_t MAGIC_HEADER = 0x465A5248; // 'HRZF' - Horizon Engine Protected Format

public:
    static bool SaveEngineProfile(const std::string& path, const FileBuffer& buffer) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;

        // 1. Explicitly guarantee the static CRC lookup tables are initialized 
        CRC32_64::init();

        // 2. Execute the exact CRC32 state lifecycle contract defined by your class
        CRC32_64 hasher;
        hasher.reset32(); // Set state to 0xFFFFFFFF 
        hasher.appendCRC32(buffer.stream.data(), buffer.stream.size()); // Append whole block 
        hasher.finalize32(); // Run the missing bitwise inversion mask (crc32 ^= 0xFFFFFFFF) 

        uint32_t computedChecksum = hasher.getCRC32(); // Extract the true finalized checksum 

        // 3. Serialize formatted stream layout to disk
        uint32_t payloadSize = static_cast<uint32_t>(buffer.stream.size());
        file.write(reinterpret_cast<const char*>(&MAGIC_HEADER), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&payloadSize), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(buffer.stream.data()), payloadSize);
        file.write(reinterpret_cast<const char*>(&computedChecksum), sizeof(uint32_t));

        return true;
    }

    static bool LoadEngineProfile(const std::string& path, std::vector<uint8_t>& outPayload) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        std::streamsize fileSize = file.tellg();
        if (fileSize < static_cast<std::streamsize>(sizeof(uint32_t) * 3)) return false;

        file.seekg(0, std::ios::beg);

        // 1. Verify Magic Header Format Identifier
        uint32_t readMagic = 0;
        file.read(reinterpret_cast<char*>(&readMagic), sizeof(uint32_t));
        if (readMagic != MAGIC_HEADER) return false;

        // 2. Verify Data Stream Boundaries
        uint32_t declaredSize = 0;
        file.read(reinterpret_cast<char*>(&declaredSize), sizeof(uint32_t));
        if (fileSize != static_cast<std::streamsize>(sizeof(uint32_t) * 3 + declaredSize)) return false;

        // 3. Pull contiguous byte strip payload into memory
        outPayload.resize(declaredSize);
        file.read(reinterpret_cast<char*>(outPayload.data()), declaredSize);

        // 4. Extract serialized checksum footer
        uint32_t fileChecksum = 0;
        file.read(reinterpret_cast<char*>(&fileChecksum), sizeof(uint32_t));

        // 5. Evaluate payload data integrity using your class lifecycle rules
        CRC32_64::init();
        CRC32_64 hasher;
        hasher.reset32(); // 
        hasher.appendCRC32(outPayload.data(), outPayload.size()); // 
        hasher.finalize32(); // 

        if (hasher.getCRC32() != fileChecksum) { // 
            outPayload.clear(); // Corrupted data drop boundary
            return false;
        }

        return true;
    }
};