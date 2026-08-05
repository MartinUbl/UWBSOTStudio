#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <limits.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>
#endif

class CShared_Library
{
public:
#if defined(_WIN32)
	using THandle = HMODULE;
#else
	using THandle = void*;
#endif

private:
	THandle m_Handle = nullptr;
	std::filesystem::path m_Path;

public:
	CShared_Library() = default;

	explicit CShared_Library(const std::filesystem::path& path)
	{
		Open(path);
	}

	~CShared_Library()
	{
		Close();
	}

	CShared_Library(const CShared_Library&) = delete;
	CShared_Library& operator=(const CShared_Library&) = delete;

	CShared_Library(CShared_Library&& other) noexcept
		: m_Handle(other.m_Handle),
		m_Path(std::move(other.m_Path))
	{
		other.m_Handle = nullptr;
	}

	CShared_Library& operator=(CShared_Library&& other) noexcept
	{
		if (this != &other)
		{
			Close();

			m_Handle = other.m_Handle;
			m_Path = std::move(other.m_Path);

			other.m_Handle = nullptr;
		}

		return *this;
	}

	void Open(const std::filesystem::path& path)
	{
		Close();

		m_Path = path;

#if defined(_WIN32)
		m_Handle = LoadLibraryW(path.wstring().c_str());
		if (!m_Handle) {
			// retrieve error code
			DWORD errorCode = GetLastError();
			throw std::runtime_error("Failed to load shared library: " + path.string() + ", error code: " + std::to_string(errorCode));
		}
#else
		m_Handle = dlopen(path.string().c_str(), RTLD_NOW);
		if (!m_Handle)
		{
			const char* err = dlerror();
			throw std::runtime_error(
				"Failed to load library: " + path.string() +
				(err ? std::string(" : ") + err : std::string())
			);
		}
#endif
	}

	void Close()
	{
		if (!m_Handle)
			return;

#if defined(_WIN32)
		FreeLibrary(m_Handle);
#else
		dlclose(m_Handle);
#endif

		m_Handle = nullptr;
	}

	bool Is_Open() const
	{
		return m_Handle != nullptr;
	}

	const std::filesystem::path& Path() const
	{
		return m_Path;
	}

	void* Resolve_Raw(const std::string& symbol_name) const
	{
		if (!m_Handle)
			throw std::runtime_error("Cannot resolve symbol from closed library");

#if defined(_WIN32)
		void* symbol = reinterpret_cast<void*>(
			GetProcAddress(m_Handle, symbol_name.c_str())
			);
#else
		dlerror(); // clear previous error
		void* symbol = dlsym(m_Handle, symbol_name.c_str());
#endif

		if (!symbol)
			throw std::runtime_error("Failed to resolve symbol: " + symbol_name);

		return symbol;
	}

	template<typename T>
	T Resolve(const std::string& symbol_name) const
	{
		return reinterpret_cast<T>(Resolve_Raw(symbol_name));
	}
};

class CShared_Library_Loader
{
private:
	std::filesystem::path m_Plugins_Directory;

public:
	CShared_Library_Loader()
		: m_Plugins_Directory(Get_Current_Executable_Directory() / "plugins")
	{
	}

	explicit CShared_Library_Loader(std::filesystem::path plugins_directory)
		: m_Plugins_Directory(std::move(plugins_directory))
	{
	}

	const std::filesystem::path& Plugins_Directory() const
	{
		return m_Plugins_Directory;
	}

	void Set_Plugins_Directory(std::filesystem::path plugins_directory)
	{
		m_Plugins_Directory = std::move(plugins_directory);
	}

	std::vector<std::filesystem::path> Enumerate() const
	{
		std::vector<std::filesystem::path> result;

		if (!std::filesystem::exists(m_Plugins_Directory))
			return result;

		for (const auto& entry : std::filesystem::directory_iterator(m_Plugins_Directory))
		{
			if (!entry.is_regular_file())
				continue;

			const auto& path = entry.path();

			if (Is_Shared_Library_File(path))
				result.push_back(path);
		}

		std::sort(result.begin(), result.end());

		return result;
	}

	std::shared_ptr<CShared_Library> Load(const std::filesystem::path& path) const
	{
		return std::make_shared<CShared_Library>(path);
	}

	std::shared_ptr<CShared_Library> Load_By_Name(const std::string& name) const
	{
		auto path = m_Plugins_Directory / Make_Platform_Library_Filename(name);
		return Load(path);
	}

private:
	static bool Is_Shared_Library_File(const std::filesystem::path& path)
	{
		const auto ext = path.extension().string();

#if defined(_WIN32)
		return ext == ".dll";
#elif defined(__APPLE__)
		return ext == ".dylib" || ext == ".so";
#else
		return ext == ".so";
#endif
	}

	static std::filesystem::path Make_Platform_Library_Filename(const std::string& name)
	{
		std::filesystem::path path{ name };

		if (path.has_extension())
			return path;

#if defined(_WIN32)
		return name + ".dll";
#elif defined(__APPLE__)
		return "lib" + name + ".dylib";
#else
		return "lib" + name + ".so";
#endif
	}

	static std::filesystem::path Get_Current_Executable_Directory()
	{
#if defined(_WIN32)

		std::wstring buffer;
		buffer.resize(MAX_PATH);

		DWORD size = 0;

		while (true)
		{
			size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

			if (size == 0)
				throw std::runtime_error("Failed to get current executable path");

			if (size < buffer.size() - 1)
				break;

			buffer.resize(buffer.size() * 2);
		}

		buffer.resize(size);

		return std::filesystem::path(buffer).parent_path();

#elif defined(__APPLE__)

		uint32_t size = 0;
		_NSGetExecutablePath(nullptr, &size);

		std::string buffer;
		buffer.resize(size);

		if (_NSGetExecutablePath(buffer.data(), &size) != 0)
			throw std::runtime_error("Failed to get current executable path");

		return std::filesystem::canonical(buffer).parent_path();

#else

		std::string buffer;
		buffer.resize(PATH_MAX);

		ssize_t size = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);

		if (size <= 0)
			throw std::runtime_error("Failed to get current executable path");

		buffer.resize(static_cast<std::size_t>(size));

		return std::filesystem::path(buffer).parent_path();

#endif
	}
};