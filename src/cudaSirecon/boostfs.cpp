#include <boost/filesystem.hpp>
#include <regex>   // requires c++11
#include <iostream>
#include <string>
#include <algorithm> // sort

static boost::filesystem::path outputDir;
// Tracks whether the caller explicitly set the output folder (via
// setOutputFolder). When true, gatherMatchingFiles() will NOT override it
// with the default "<input-folder>/GPUsirecon" subfolder.
static bool outputDirOverridden = false;

static void createOutputDirIfNeeded(const boost::filesystem::path& dir)
{
  namespace bfs = boost::filesystem;
  if (dir.empty() || bfs::exists(dir)) return;

  boost::system::error_code ec;
  bfs::create_directories(dir, ec);
  if (ec) {
    std::cerr << "Warning: could not create output dir "
              << dir.string() << ": " << ec.message() << std::endl;
    return;
  }
  // Default permissions 0775 so group members (shared analysis accounts on
  // cluster filesystems) can read and write into it; the process umask would
  // otherwise typically strip group-write.
  bfs::permissions(
      dir,
      bfs::owner_all |
      bfs::group_all |
      bfs::others_read |
      bfs::others_exe,
      ec);
  if (ec) {
    std::cerr << "Warning: could not set 0775 permissions on "
              << dir.string() << ": " << ec.message() << std::endl;
  }
}

void setOutputFolder(const std::string& path)
{
  if (path.empty()) {
    outputDirOverridden = false;
    outputDir.clear();
    return;
  }
  outputDir = boost::filesystem::path(path);
  outputDirOverridden = true;
  createOutputDirIfNeeded(outputDir);
}

std::vector<std::string> gatherMatchingFiles(std::string target_path, std::string pattern)
{
  // Check if the pattern is specified as a full file name (i.e.; if '.tif' is in the name)
  size_t p1 = pattern.rfind(".tif");
  size_t p2 = pattern.rfind(".TIF");
  if (p1 != std::string::npos || p2 != std::string::npos) {
    // if no matches were found, rfind() returns 'string::npos'
    // do not append ".*tif" at the end of pattern
  }
  else
    pattern.append(".*\\.[tT][iI][fF]");
  pattern.insert(0, ".*");  // '.' is the wildcard in Perl regexp; '*' just means "repeat".

  const std::regex my_filter(pattern);

  std::vector< std::string > all_matching_files;

  boost::filesystem::directory_iterator end_itr; // Constructs the end iterator.
  for (boost::filesystem::directory_iterator i(target_path); i != end_itr; ++i) {

    // Skip if not a file
    if( !boost::filesystem::is_regular_file( i->status() ) ) continue;

    std::smatch what;

    auto fname = i->path().string(); //somehow this local variable is necessary with VS2017
    // With GCC 7.5 this variable is not needed; in-place "i->path().string()" is enough

    // Skip if no match
    if( !std::regex_match(fname/*i->path().string()*/, what, my_filter) ) continue;

    // File matches, store it
    all_matching_files.push_back(fname /*i->path().string()*/);
  }

  // sort file names so that earlier time points will be processed first:
  sort(all_matching_files.begin(), all_matching_files.end());


  // If the caller hasn't overridden the output directory (via setOutputDir),
  // fall back to the historical default of "<input-folder>/GPUsirecon/".
  if (!outputDirOverridden) {
    outputDir = target_path;
    outputDir /= "GPUsirecon";
    createOutputDirIfNeeded(outputDir);
  }

  return all_matching_files;
}


std::string makeOutputFilePath(std::string inputFileName, std::string insert)
{
  boost::filesystem::path inputpath(inputFileName);
  boost::filesystem::path outputpath(outputDir);

  std::string basename = inputpath.filename().string();
  int pos = basename.find_last_of(".tif");
  basename.insert(pos - 3, insert);

  outputpath /= basename;

  std::cout << "Output: " << outputpath.string() << '\n';
  return outputpath.string();
}
