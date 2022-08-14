#include <catch2/catch_all.hpp>

#include <spdlog/spdlog.h>
#include <chrono>
//#include <texpress/api.hpp>


struct ExportTable {
public:
  ExportTable() = delete;
  std::string table_name;

  ExportTable(std::string name, size_t max_rows) :
    table_name{ name }
    , headers{}
    , content(new std::vector<std::string>[max_rows])
    , content_size(0)
    , max_size(max_rows)
  {
    reserve(10);
  }

  size_t size() {
    return content_size;
  }

  size_t curr_id() {
    return content_size - 1;
  }

  void reserve(size_t size) {
    for (size_t i = 0; i < max_size; i++) {
      content[i].reserve(size);
    }
  }

  long int add_row() {
    if (content_size < max_size) {
      -1;
    }

    for (const auto& h : headers) {
      content[content_size].push_back("");
    }

    content_size++;
    return content_size - 1;
  }

  void add_data(long int row_key, const std::string& header_value, const std::string& data_value) {
    if (row_key >= size()) {
      return;
    }

    auto header_id = find_header_id(header_value);

    // Create header if necessary
    if (header_id < 0) {
      headers.push_back(header_value);
      header_id = headers.size() - 1;

      for (size_t i = 0; i < size(); i++) {
        content[i].push_back("");
      }
    }

    content[row_key][header_id] += data_value;
  }

  std::string build() {
    std::string table = "";
    for (size_t i = 0; i < headers.size(); i++) {
      table += headers[i];

      if (i != headers.size() - 1) {
        table += ',';
      }
    }
    table += '\n';

    for (size_t i = 0; i < size(); i++) {
      for (size_t j = 0; j < content[i].size(); j++) {
        table += content[i][j];

        if (j < content[i].size() - 1) {
          table += ",";
        }
      }

      table += '\n';
    }

    return table;
  }

private:
  long int find_header_id(const std::string& header_value) {
    for (long int i = 0; i < headers.size(); i++) {
      if (headers[i] == header_value) {
        return i;
      }
    }

    return -1;
  }

private:
  std::vector<std::string> headers; // columns
  std::vector<std::string>* content; // rows

  size_t content_size;
  size_t max_size;
};

TEST_CASE("Sandbox testing.", "[texpress::sandbox]")
{
  auto table = ExportTable("TestTable", 10000);
  table.reserve(50);

  auto table_t0 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 1000; i++) {
    auto curr_row = table.add_row();
    for (int j = 0; j < 10; j++) {
      table.add_data(table.curr_id(), "Column " + std::to_string(j), std::to_string(i));
    }
  }
  auto table_t1 = std::chrono::high_resolution_clock::now();
  spdlog::info("Table: {0}", std::chrono::duration_cast<std::chrono::microseconds>(table_t1 - table_t0).count());
}