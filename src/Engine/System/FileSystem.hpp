#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <concepts>
#include "../Utils/CRC32_64/CRC32_64.hpp"

// ============================================================================
// BINARY WRITER / SERIALIZER STREAM
// ============================================================================
class BinarySerializer {
private:
    std::vector<uint8_t> m_stream;

public:
    BinarySerializer() { m_stream.reserve(256); }

    void clear() { m_stream.clear(); }
    [[nodiscard]] const std::vector<uint8_t>& getBuffer() const noexcept { return m_stream; }
    [[nodiscard]] size_t size() const noexcept { return m_stream.size(); }

    // --- Raw Byte Operations ---
    void writeBytes(const void* data, size_t size) {
        if (!data || size == 0) return;
        const auto* bytePtr = static_cast<const uint8_t*>(data);
        m_stream.insert(m_stream.end(), bytePtr, bytePtr + size);
    }

    // --- Primitive Types (int, float, bool, double, glm::vec2, etc.) ---
    template<typename T>
        requires std::is_trivially_copyable_v<T>
    BinarySerializer& write(const T& value) {
        writeBytes(&value, sizeof(T));
        return *this;
    }

    // --- Strings ---
    BinarySerializer& writeString(const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.size());
        write(len);
        if (len > 0) {
            writeBytes(str.data(), len);
        }
        return *this;
    }

    // --- Vectors ---
    template<typename T>
    BinarySerializer& writeVector(const std::vector<T>& vec) {
        uint32_t count = static_cast<uint32_t>(vec.size());
        write(count);

        if constexpr (std::is_trivially_copyable_v<T>) {
            if (count > 0) {
                writeBytes(vec.data(), count * sizeof(T));
            }
        }
        else {
            for (const auto& item : vec) {
                writeObject(item);
            }
        }
        return *this;
    }

    // --- Maps / Key-Value Pairs ---
    template<typename K, typename V>
    BinarySerializer& writeMap(const std::unordered_map<K, V>& map) {
        uint32_t count = static_cast<uint32_t>(map.size());
        write(count);
        for (const auto& [key, value] : map) {
            writeObject(key);
            writeObject(value);
        }
        return *this;
    }

    // --- Generic Object Delegate ---
    template<typename T>
    BinarySerializer& writeObject(const T& obj) {
        if constexpr (requires { obj.serialize(*this); }) {
            obj.serialize(*this); // Call member method if object implements it
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            writeString(obj);
        }
        else if constexpr (std::is_trivially_copyable_v<T>) {
            write(obj);
        }
        else {
            static_assert(sizeof(T) == 0, "Type does not have a valid serialization route!");
        }
        return *this;
    }

    // Modern C++ Operator Chaining Engine
    template<typename T>
    BinarySerializer& operator<<(const T& data) {
        return writeObject(data);
    }
};

// Backward-compatibility alias for existing code
using FileBuffer = BinarySerializer;

// ============================================================================
// BINARY READER / DESERIALIZER STREAM
// ============================================================================
class BinaryDeserializer {
private:
    std::vector<uint8_t> m_buffer;
    size_t m_readOffset = 0;
    bool m_hasError = false;

public:
    BinaryDeserializer() = default;
    explicit BinaryDeserializer(std::vector<uint8_t> buffer) : m_buffer(std::move(buffer)), m_readOffset(0) {}

    explicit operator bool() const noexcept { return !m_hasError && m_readOffset <= m_buffer.size(); }
    bool operator!() const noexcept { return m_hasError; }

    void setBuffer(std::vector<uint8_t> buffer) {
        m_buffer = std::move(buffer);
        m_readOffset = 0;
    }

    [[nodiscard]] size_t getRemainingBytes() const noexcept {
        return (m_readOffset < m_buffer.size()) ? m_buffer.size() - m_readOffset : 0;
    }

    [[nodiscard]] bool hasBytes(size_t bytes) const noexcept {
        return m_readOffset + bytes <= m_buffer.size();
    }

    // --- Raw Byte Reading with Safe Bounds Check ---
    bool readBytes(void* outData, size_t size) {
        if (!hasBytes(size)) {
            m_hasError = true;
            return false;
        }
        std::memcpy(outData, m_buffer.data() + m_readOffset, size);
        m_readOffset += size;
        return true;
    }

    // --- Primitive Types ---
    template<typename T>
        requires std::is_trivially_copyable_v<T>
    bool read(T& outValue) {
        return readBytes(&outValue, sizeof(T));
    }

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    T readOr(T fallback = T{}) {
        T val;
        if (read(val)) return val;
        return fallback;
    }

    // --- Strings ---
    bool readString(std::string& outStr) {
        uint32_t len = 0;
        if (!read(len)) return false;
        if (len == 0) { outStr.clear(); return true; }
        if (!hasBytes(len)) return false;

        outStr.resize(len);
        std::memcpy(outStr.data(), m_buffer.data() + m_readOffset, len);
        m_readOffset += len;
        return true;
    }

    // --- Vectors ---
    template<typename T>
    bool readVector(std::vector<T>& outVec) {
        uint32_t count = 0;
        if (!read(count)) return false;
        outVec.clear();
        outVec.reserve(count);

        if constexpr (std::is_trivially_copyable_v<T>) {
            size_t totalBytes = count * sizeof(T);
            if (!hasBytes(totalBytes)) return false;
            outVec.resize(count);
            return readBytes(outVec.data(), totalBytes);
        }
        else {
            for (uint32_t i = 0; i < count; ++i) {
                T item;
                if (!readObject(item)) return false;
                outVec.push_back(std::move(item));
            }
            return true;
        }
    }

    // --- Maps / Key-Value Pairs ---
    template<typename K, typename V>
    bool readMap(std::unordered_map<K, V>& outMap) {
        uint32_t count = 0;
        if (!read(count)) return false;
        outMap.clear();
        outMap.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            K key;
            V val;
            if (!readObject(key) || !readObject(val)) return false;
            outMap[key] = val;
        }
        return true;
    }

    // --- Generic Object Delegate ---
    template<typename T>
    bool readObject(T& outObj) {
        if constexpr (requires { outObj.deserialize(*this); }) {
            return outObj.deserialize(*this);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return readString(outObj);
        }
        else if constexpr (std::is_trivially_copyable_v<T>) {
            return read(outObj);
        }
        else {
            static_assert(sizeof(T) == 0, "Type does not have a valid deserialization route!");
            return false;
        }
    }

    // Modern C++ Operator Chaining Engine
    template<typename T>
    BinaryDeserializer& operator>>(T& target) {
        if (!readObject(target)) {
            m_hasError = true;
        }
        return *this;
    }
};

// ==========================================
// FILESYSTEM STORAGE & INTEGRITY MANAGER
// ==========================================
class FileSystem {
public:
    static constexpr uint32_t ENGINE_MAGIC_HEADER = 0x465A5248; // 'HRZF' - Horizon Engine Protected
    static constexpr uint32_t GAME_SAVE_MAGIC = 0x53415645; // 'SAVE' - Horizon Game Save State

    // --- Generic Binary Save Engine ---
    static bool SaveToFile(const std::string& path, const BinarySerializer& serializer, uint32_t magic = GAME_SAVE_MAGIC) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;

        const auto& streamData = serializer.getBuffer();

        // 1. Explicitly initialize static CRC lookup tables
        CRC32_64::init();

        // 2. Execute exact CRC32 state lifecycle contract
        CRC32_64 hasher;
        hasher.reset32();
        hasher.appendCRC32(streamData.data(), streamData.size());
        hasher.finalize32();

        uint32_t computedChecksum = hasher.getCRC32();

        // 3. Serialize formatted stream layout to disk
        uint32_t payloadSize = static_cast<uint32_t>(streamData.size());
        file.write(reinterpret_cast<const char*>(&magic), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&payloadSize), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(streamData.data()), payloadSize);
        file.write(reinterpret_cast<const char*>(&computedChecksum), sizeof(uint32_t));

        return true;
    }

    // --- Generic Binary Load Engine ---
    static bool LoadFromFile(const std::string& path, BinaryDeserializer& outDeserializer, uint32_t expectedMagic = GAME_SAVE_MAGIC) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        std::streamsize fileSize = file.tellg();
        if (fileSize < static_cast<std::streamsize>(sizeof(uint32_t) * 3)) return false;

        file.seekg(0, std::ios::beg);

        // 1. Verify Magic Header Format Identifier
        uint32_t readMagic = 0;
        file.read(reinterpret_cast<char*>(&readMagic), sizeof(uint32_t));
        if (readMagic != expectedMagic) return false;

        // 2. Verify Data Stream Boundaries
        uint32_t declaredSize = 0;
        file.read(reinterpret_cast<char*>(&declaredSize), sizeof(uint32_t));
        if (fileSize != static_cast<std::streamsize>(sizeof(uint32_t) * 3 + declaredSize)) return false;

        // 3. Pull contiguous byte strip payload into memory
        std::vector<uint8_t> payload(declaredSize);
        file.read(reinterpret_cast<char*>(payload.data()), declaredSize);

        // 4. Extract serialized checksum footer
        uint32_t fileChecksum = 0;
        file.read(reinterpret_cast<char*>(&fileChecksum), sizeof(uint32_t));

        // 5. Evaluate payload data integrity
        CRC32_64::init();
        CRC32_64 hasher;
        hasher.reset32();
        hasher.appendCRC32(payload.data(), payload.size());
        hasher.finalize32();

        if (hasher.getCRC32() != fileChecksum) {
            return false; // Reject corrupted save file
        }

        // Hydrate deserializer stream
        outDeserializer.setBuffer(std::move(payload));
        return true;
    }

    // --- Backward Compatibility Wrappers ---
    static bool SaveEngineProfile(const std::string& path, const FileBuffer& buffer) {
        return SaveToFile(path, buffer, ENGINE_MAGIC_HEADER);
    }

    static bool LoadEngineProfile(const std::string& path, std::vector<uint8_t>& outPayload) {
        BinaryDeserializer deserializer;
        if (!LoadFromFile(path, deserializer, ENGINE_MAGIC_HEADER)) return false;
        outPayload = deserializer.getRemainingBytes() > 0 ?
            std::vector<uint8_t>(deserializer.getRemainingBytes()) : std::vector<uint8_t>();
        deserializer.readBytes(outPayload.data(), outPayload.size());
        return true;
    }
};