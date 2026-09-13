#ifndef OUI_DATABASE_H
#define OUI_DATABASE_H

#include <string>
#include <unordered_map>
#include <utility>

/**
 * @brief Tra cứu thông tin nhà sản xuất và loại thiết bị từ 6 ký tự OUI MAC
 */
const std::unordered_map<std::string, std::pair<std::string, std::string>>& getOuiDatabase();

#endif // OUI_DATABASE_H
