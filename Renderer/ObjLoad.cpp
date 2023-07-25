#include "ObjLoad.h"

#include <cassert>
#include <fstream>
#include <string>
#include <sstream>

ObjData LoadObj(const std::filesystem::path& path) {
    ObjData objData;
    std::vector<Vector3> positions;
    std::vector<Vector3> normals;

    std::ifstream file(path);
    assert(file.is_open());

    std::string line;
    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream iss(line);
        iss >> identifier;

        // コメントをスキップ
        if (identifier == "#") {
            continue;
        }
        // メッシュを追加
        else if (identifier == "v") {
            Vector3& position = positions.emplace_back();
            iss >> position.x >> position.y >> position.z;
        }
        else if (identifier == "vn") {
            Vector3& normal = normals.emplace_back();
            iss >> normal.x >> normal.y >> normal.z;
        }
        else if (identifier == "f") {
            for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
                std::string vertexDefinition;
                iss >> vertexDefinition;

                std::istringstream viss(vertexDefinition);
                size_t elementIndices[3]{};
                bool loaded[3]{ false };
                size_t indexCount = 0;
                for (int32_t element = 0; element < 3; ++element) {
                    std::string index;
                    std::getline(viss, index, '/');
                    if (index != "") {
                        elementIndices[element] = static_cast<size_t>(std::stoi(index));
                        ++indexCount;
                        loaded[element] = true;
                    }
                }
                if (indexCount == 2 && !loaded[2]) {
                    elementIndices[2] = elementIndices[1];
                }
                Vector3& position = positions[elementIndices[0] - 1];
                Vector3& normal = normals[elementIndices[2] - 1];
                objData.positions.emplace_back(position);
                objData.normals.emplace_back(normal);
                objData.indices.emplace_back(uint16_t(objData.indices.size()));

            }
        }
    }
    return objData;
}
