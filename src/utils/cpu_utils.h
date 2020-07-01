#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <limits>
#include <libpmem.h>
#include <fstream>
#include <xmmintrin.h>

using namespace std;

inline bool file_exists (const std::string& name) {
  ifstream f(name.c_str());
  return f.good();
}

template<typename T>
T* create_pmem_buffer(string name, size_t num_items) {
  size_t buffer_size = sizeof(T) * num_items;
  string path = "/mnt/pmem13/";
  path += name;

  char* pmemaddr;
  int is_pmem;
  size_t mapped_len;
  if (file_exists(path)) {
    pmemaddr = (char*)pmem_map_file(path.c_str(), buffer_size,
        PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
  } else {
    pmemaddr = (char*)pmem_map_file(path.c_str(), buffer_size,
        PMEM_FILE_CREATE|PMEM_FILE_EXCL, 0666, &mapped_len, &is_pmem);
  }

  if (pmemaddr == NULL) {
    perror("pmem_map_file failed");
    exit(1);
  }

  if (is_pmem) {
    cout << "pmem successfully allocated" << endl;
  } else {
    cout << "error not pmem" << endl;
    exit(1);
  }

  T* buf = reinterpret_cast<T*>(pmemaddr);
  return buf;
}

template<typename T>
T* create_buffer(string name, size_t num_items, string type) {
  if (type == "pmem") {
    return create_pmem_buffer<T>(name, num_items);
  } else {
    return (T*) _mm_malloc(sizeof(T) * num_items, 256);
  }
}

template<typename T>
void destroy_buffer(T*& buf, size_t num_items, string type) {
  if (type == "pmem") {
    pmem_unmap(buf, sizeof(T) * num_items);
  } else {
    _mm_free(buf);
  }
}

template<typename T>
void persist_buffer(T*& buf, size_t num_items, string type) {
  if (type == "pmem") {
    cout << sizeof(T) << endl;
    pmem_persist(buf, sizeof(T) * num_items);
  }
}

/**
 * Utility for parsing command line arguments
 */
struct CommandLineArgs
{

  std::vector<std::string>  keys;
  std::vector<std::string>  values;
  std::vector<std::string>  args;

  /**
   * Constructor
   */
  CommandLineArgs(int argc, char **argv) :
    keys(10),
    values(10)
  {
    using namespace std;

    for (int i = 1; i < argc; i++)
    {
      string arg = argv[i];

      if ((arg[0] != '-') || (arg[1] != '-'))
      {
        args.push_back(arg);
        continue;
      }

      string::size_type pos;
      string key, val;
      if ((pos = arg.find('=')) == string::npos) {
        key = string(arg, 2, arg.length() - 2);
        val = "";
      } else {
        key = string(arg, 2, pos - 2);
        val = string(arg, pos + 1, arg.length() - 1);
      }

      keys.push_back(key);
      values.push_back(val);
    }
  }


  /**
   * Checks whether a flag "--<flag>" is present in the commandline
   */
  bool CheckCmdLineFlag(const char* arg_name)
  {
    using namespace std;

    for (int i = 0; i < int(keys.size()); ++i)
    {
      if (keys[i] == string(arg_name))
        return true;
    }
    return false;
  }


  /**
   * Returns number of naked (non-flag and non-key-value) commandline parameters
   */
  template <typename T>
  int NumNakedArgs()
  {
    return args.size();
  }


  /**
   * Returns the commandline parameter for a given index (not including flags)
   */
  template <typename T>
  void GetCmdLineArgument(int index, T &val)
  {
    using namespace std;
    if (index < args.size()) {
      istringstream str_stream(args[index]);
      str_stream >> val;
    }
  }

  /**
   * Returns the value specified for a given commandline parameter --<flag>=<value>
   */
  template <typename T>
  void GetCmdLineArgument(const char *arg_name, T &val)
  {
    using namespace std;

    for (int i = 0; i < int(keys.size()); ++i)
    {
      if (keys[i] == string(arg_name))
      {
        istringstream str_stream(values[i]);
        str_stream >> val;
      }
    }
  }


  /**
   * Returns the values specified for a given commandline parameter --<flag>=<value>,<value>*
   */
  template <typename T>
  void GetCmdLineArguments(const char *arg_name, std::vector<T> &vals)
  {
    using namespace std;

    if (CheckCmdLineFlag(arg_name))
    {
      // Clear any default values
      vals.clear();

      // Recover from multi-value string
      for (int i = 0; i < keys.size(); ++i)
      {
        if (keys[i] == string(arg_name))
        {
          string val_string(values[i]);
          istringstream str_stream(val_string);
          string::size_type old_pos = 0;
          string::size_type new_pos = 0;

          // Iterate comma-separated values
          T val;
          while ((new_pos = val_string.find(',', old_pos)) != string::npos)
          {
            if (new_pos != old_pos)
            {
              str_stream.width(new_pos - old_pos);
              str_stream >> val;
              vals.push_back(val);
            }

            // skip over comma
            str_stream.ignore(1);
            old_pos = new_pos + 1;
          }

          // Read last value
          str_stream >> val;
          vals.push_back(val);
        }
      }
    }
  }


  /**
   * The number of pairs parsed
   */
  int ParsedArgc()
  {
    return (int) keys.size();
  }
};
