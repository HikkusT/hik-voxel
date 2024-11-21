#include "VoxLoader.h"
#define OGT_VOX_IMPLEMENTATION
#include "../vendor/ogt_vox.h"
#include "spdlog/spdlog.h"
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace cubik {
  int pow2roundup(int x)
  {
    if (x < 0)
      return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
  }

  std::vector<int> loadVoxFile(const char *filename, int& size) {
    FILE * fp;
    if (0 != fopen_s(&fp, filename, "rb"))
      fp = 0;
    if (!fp) {
      spdlog::error("Failed to open file {}", filename);
      abort();
    }

    // get the buffer size which matches the size of the file
    fseek(fp, 0, SEEK_END);
    uint32_t buffer_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // load the file into a memory buffer
    auto * buffer = new uint8_t[buffer_size];
    fread(buffer, buffer_size, 1, fp);
    fclose(fp);

    // construct the scene from the buffer
    const ogt_vox_scene* scene = ogt_vox_read_scene_with_flags(buffer, buffer_size, k_read_scene_flags_groups);
    const ogt_vox_model* model = scene->models[0];

    // the buffer can be safely deleted once the scene is instantiated.
    delete[] buffer;

    auto minBounds = glm::ivec3(0);
    auto maxBounds = glm::ivec3(0);
    for (int i = 0; i < scene->num_models; i++) {
      auto currentModel = scene->models[i];
      auto currentInstance = scene->instances[i];
      auto modelSize = glm::ivec3(currentModel->size_x, currentModel->size_y, currentModel->size_z);
      auto position = glm::ivec3(currentInstance.transform.m30, currentInstance.transform.m31, currentInstance.transform.m32);

      minBounds = glm::min(position, minBounds);
      maxBounds = glm::max(position + modelSize, maxBounds);
    }
    auto totalSize = maxBounds - minBounds;
    spdlog::info("Bounds from {} to {}. So true size is {}", glm::to_string(minBounds), glm::to_string(maxBounds), glm::to_string(totalSize));

//    size = pow2roundup((int) std::max(model->size_x, std::max(model->size_y, model->size_z)));
//    std::vector<int> voxelData(size * size * size, 0);
//    for (int x = 0; x < size; x++) {
//      for (int y = 0; y < size; y++) {
//        for (int z = 0; z < size; z++) {
//          int index = y * size * size + (size - 1 - z) * size + x;
//          if (x < model->size_x && y < model->size_y && z < model->size_z) {
//            voxelData[index] = model->voxel_data[x + (y * model->size_x) + (z * model->size_x * model->size_y)] != 0;
//          } else {
//            voxelData[index] = 0;
//          }
//        }
//      }
//    }

    size = pow2roundup((int) std::max(totalSize.x, std::max(totalSize.y, totalSize.z)));
    std::vector<int> voxelData(size * size * size, 0);

    for (int i = 0; i < scene->num_models; i++) {
      auto currentModel = scene->models[i];
      auto currentInstance = scene->instances[i];
      auto modelSize = glm::ivec3(currentModel->size_x, currentModel->size_y, currentModel->size_z);
      auto position = glm::ivec3(currentInstance.transform.m30, currentInstance.transform.m31, currentInstance.transform.m32);

      int iterationSize = (int) std::max(modelSize.x, std::max(modelSize.y, modelSize.z));
      for (int x = 0; x < iterationSize; x++) {
        for (int y = 0; y < iterationSize; y++) {
          for (int z = 0; z < iterationSize; z++) {
            int index = (y + position.y - minBounds.y) * size * size + (size - 1 - (z + position.z - minBounds.z)) * size + (x + position.x - minBounds.x);
            if (x < modelSize.x && y < modelSize.y && z < modelSize.z) {
              voxelData[index] = currentModel->voxel_data[x + (y * currentModel->size_x) + (z * currentModel->size_x * currentModel->size_y)] != 0;
            }
          }
        }
      }
    }

    spdlog::info("loaded: {} / {} {} {}", size, model->size_x, model->size_y, model->size_z);
    return voxelData;
  }
}
