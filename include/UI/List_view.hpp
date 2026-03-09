#pragma once

#include <borealis.hpp>

#include <string>
#include "common.hpp"


#define CELL_NAME_HOME "HomeCell"
#define CELL_NAME_SETTINGS "SettingsCell"
#define CELL_NAME_FILE "FileCell"


class ImgTextCell
{
  public:
    std::string text;
    std::string imageRes;

    ImgTextCell(std::string text, std::string imageRes)
        : text(text)
        , imageRes(imageRes)
    {
    }
};



class RecyclerCell
    : public brls::RecyclerCell
{
  public:
    RecyclerCell();

    void onFocusGained() override;
    void onFocusLost() override;



    BRLS_BIND(brls::Rectangle, accent, "brls/sidebar/item_accent");
    BRLS_BIND(brls::Label, label, "title");
    BRLS_BIND(brls::Image, image, "image");

    static RecyclerCell* create();
};

class FileListView;
class DataSource
    : public brls::RecyclerDataSource
{
  private:
    std::string cell_name;
  public:
    std::vector<ImgTextCell> listItems;



    void setCellName(std::string name);
    int numberOfSections(brls::RecyclerFrame* recycler) override;
    int numberOfRows(brls::RecyclerFrame* recycler, int section) override;
    RecyclerCell* cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override;
    void didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath) override;
    std::string titleForHeader(brls::RecyclerFrame* recycler, int section) override;
};


class HomeMenuListView;


class ListView : public brls::Box
{
  public:
    ListView();
    ~ListView();
    // static brls::View* create();

    
    BRLS_BIND(brls::RecyclerFrame, recycler, "recycler");
    DataSource* dataSource;

    std::vector<ImgTextCell> getItems();
    void setCellName(std::string name);
    void addItem(std::string title, std::string imageRes);
    void clearItems();
    void applyItems();
  private:
    std::string cell_name;
    
};



class HomeMenuListView : public ListView
{
  public:
    HomeMenuListView();
    ~HomeMenuListView();
    static brls::View* create();
};

// class SettingsListView : public ListView
// {
//   public:
//     SettingsListView();
//     ~SettingsListView();
//     static brls::View* create();
// };


class FileListView : public ListView
{
  public:
    FileListView();
    ~FileListView();
    static brls::View* create();

    // 过滤函数，判断文件是否符合过滤条件， 符合条件返回 true，否则返回 false
  bool FileFliter(std::string fileSuffix);
  void ListCurrentDir(std::string path);

  std::vector<std::string> filterList = {
    "gba", "gbc", "gb", "zip", "png"
  };

  private:
    bool isFilterEnabled = true;

};