#pragma once

#include <cstdint>
#include <vector>

enum class NOpcode : uint16_t;

class CPacket;

template<typename T>
concept TSerializable = requires(T a, CPacket& pkt) {
	a.Serialize_To(pkt);
	a.Deserialize_From(pkt);
};

template<typename T>
concept TNot_Serializable = !TSerializable<T>;

class CPacket {
	private:
		std::vector<uint8_t> mData;

		size_t mRead_Offset = 0;

	public:
		CPacket() {}

		CPacket(const CPacket& other) : mData(other.mData), mRead_Offset(other.mRead_Offset) {}
		CPacket(CPacket&& other) noexcept : mData(std::move(other.mData)), mRead_Offset(other.mRead_Offset) {}
		CPacket& operator=(const CPacket& other) {
			if (this != &other) {
				mData = other.mData;
				mRead_Offset = other.mRead_Offset;
			}
			return *this;
		}
		CPacket& operator=(CPacket&& other) noexcept {
			if (this != &other) {
				mData = std::move(other.mData);
				mRead_Offset = other.mRead_Offset;
			}
			return *this;
		}

		CPacket(const char* raw_data, size_t length) {
			assign(reinterpret_cast<const uint8_t*>(raw_data), length);
		}

		CPacket(const std::vector<char>& raw_data) {
			assign(reinterpret_cast<const uint8_t*>(raw_data.data()), raw_data.size());
		}

		CPacket(const uint8_t* raw_data, size_t length) {
			assign(raw_data, length);
		}

		CPacket(const std::vector<uint8_t>& raw_data) {
			assign(raw_data);
		}

		void assign(const uint8_t* raw_data, size_t length) {
			if (length < sizeof(uint16_t) * 2) {
				throw std::invalid_argument("Raw data too short");
			}
			mData.insert(mData.end(), raw_data, raw_data + length);
		}
		
		void assign(const std::vector<uint8_t>& raw_data) {
			if (raw_data.size() < sizeof(uint16_t) * 2) {
				throw std::invalid_argument("Raw data too short");
			}
			mData = raw_data;
		}

		void assign(const char* raw_data, size_t length) {
			if (length < sizeof(uint16_t) * 2) {
				throw std::invalid_argument("Raw data too short");
			}
			mData.insert(mData.end(), reinterpret_cast<const uint8_t*>(raw_data), reinterpret_cast<const uint8_t*>(raw_data) + length);
		}

		void assign(const std::vector<char>& raw_data) {
			if (raw_data.size() < sizeof(uint16_t) * 2) {
				throw std::invalid_argument("Raw data too short");
			}
			mData.insert(mData.end(), reinterpret_cast<const uint8_t*>(raw_data.data()), reinterpret_cast<const uint8_t*>(raw_data.data()) + raw_data.size());
		}

		const std::vector<uint8_t>& Get_Raw_Data() const {
			return mData;
		}

		template<TNot_Serializable T>
		void Put(const T& data) {
			const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);
			mData.insert(mData.end(), ptr, ptr + sizeof(T));
		}

		template<TSerializable T>
		void Put(const T& data) {
			data.Serialize_To(*this);
		}

		template<TNot_Serializable T>
		void Put_At(const T& data, size_t offset) {
			if (offset + sizeof(T) > mData.size()) {
				throw std::out_of_range("Offset exceeds data size");
			}
			std::memcpy(mData.data() + offset, &data, sizeof(T));
		}

		template<TSerializable T>
		void Put_At(const T& data, size_t offset) {
			CPacket temp_pkt;
			data.Serialize_To(temp_pkt);
			if (offset + temp_pkt.mData.size() > mData.size()) {
				throw std::out_of_range("Offset exceeds data size");
			}
			std::memcpy(mData.data() + offset, temp_pkt.mData.data(), temp_pkt.mData.size());
		}

		template<TNot_Serializable T>
		T Get() {
			if (mRead_Offset + sizeof(T) > mData.size()) {
				throw std::out_of_range("Read offset exceeds data size");
			}
			T value;
			std::memcpy(&value, mData.data() + mRead_Offset, sizeof(T));
			mRead_Offset += sizeof(T);
			return value;
		}

		template<TSerializable T>
		T Get() {
			T value;
			value.Deserialize_From(*this);
			return value;
		}

		template<typename T>
		void Put_Array(const std::vector<T>& data) {
			Put<uint16_t>(static_cast<uint16_t>(data.size()));
			for (const auto& item : data) {
				Put(item);
			}
		}

		template<typename T>
		std::vector<T> Get_Array() {
			uint16_t count = Get<uint16_t>();
			std::vector<T> result;
			result.reserve(count);
			for (size_t i = 0; i < count; ++i) {
				result.push_back(Get<T>());
			}
			return result;
		}
};
