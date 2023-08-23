#pragma once
#include <fstream>
#include <texpress/types/texture.hpp>
#include <array>

// Thanks to https://stackoverflow.com/questions/105252
template <typename T>
void SwapEnd(T& var)
{
    char* varArray = reinterpret_cast<char*>(&var);
    for (long i = 0; i < static_cast<long>(sizeof(var) / 2); i++)
        std::swap(varArray[sizeof(var) - 1 - i], varArray[i]);
}

template <typename T>
struct vtk_dataset {
    T* data;
    std::string data_title;
    int data_components;
    std::string data_type;
};

struct vtk_points {
    std::string title;
    int nx, ny, nz;
    int sx, sy, sz;
};

// int nx, int ny, int nz, int sx, int sy, int sz, std::vector<float> &scalarData)
template <typename Terr>
bool save_vtk(std::string path, const vtk_points& points, const std::vector<vtk_dataset<Terr>>& err_datasets, bool binary) {
    int numPoints = points.nx * points.ny * points.nz;

    // Open File
    std::ofstream outVTK;

    if (binary) {
        outVTK.open(path, std::ios::out | std::ios::binary);
    }
    else {
        outVTK.open(path);
    }

    if (outVTK.fail()) return 0;

    // ====================================================
    //                      HEADER
    // ====================================================

    outVTK << "# vtk DataFile Version 4.0\n"
        << points.title << "\n"
        << (binary ? "BINARY" : "ASCII") << "\n"
        << "DATASET STRUCTURED_POINTS" << "\n"
        << "DIMENSIONS " << points.nx << " " << points.ny << " " << points.nz << " " << "\n"
        << "ORIGIN 0 0 0" << "\n"
        << "SPACING " << points.sx << " " << points.sy << " " << points.sz << "\n";

    // ====================================================
    //     DATASET ATTRIBUTES (POINT DATA/VERTEX DATA)
    // ====================================================

    outVTK << "POINT_DATA " << numPoints << std::endl;

    for (const auto& err_ds : err_datasets) {
        bool vectors = false;
        if (err_ds.data_components == 3) {
            outVTK << "VECTORS " << err_ds.data_title << " " << err_ds.data_type << std::endl;
            vectors = true;
        }
        else {
            outVTK << "SCALARS " << err_ds.data_title << " " << err_ds.data_type << " " << err_ds.data_components << std::endl;
            outVTK << "LOOKUP_TABLE default" << std::endl;
        }

        for (auto point_idx = 0; point_idx < numPoints; point_idx++) {
            for (auto cmp = 0; cmp < err_ds.data_components; cmp++) {
                auto error = err_ds.data[point_idx * err_ds.data_components + cmp];

                if (binary) {
                    SwapEnd(error);

                    outVTK.write(reinterpret_cast<char*>(&error), sizeof(Terr));
                }
                else {
                    // Write error
                    outVTK << error;

                    // If vector, check whether to write space or endline
                    if (vectors) {
                        if (cmp == 2) {
                            outVTK << std::endl;
                        }
                        else {
                            outVTK << " ";
                        }
                    }
                    // Else (scalar) always write endline
                    else {
                        outVTK << std::endl;
                    }

                }
            }
        }
    }

    return 1;
}


// int nx, int ny, int nz, int sx, int sy, int sz, std::vector<float> &scalarData)
namespace texpress {
    bool save_vtk(const float* scalarData, const char* path, const char* vtk_title, int components, int nx, int ny, int nz, int sx = 1, int sy = 1, int sz = 1) {
        // Open File
        std::ofstream outStream(path);
        if (outStream.fail()) return 0;

        int numPoints = nx * ny * nz * components;

        // Writing File Header
        outStream << "# vtk DataFile Version 4.0\n" << vtk_title << "\nASCII\nDATASET STRUCTURED_POINTS" << std::endl;
        outStream << "DIMENSIONS " << nx << " " << ny << " " << nz << " " << std::endl;
        outStream << "ORIGIN 0 0 0" << std::endl;
        outStream << "SPACING " << sx << " " << sy << " " << sz << std::endl;
        outStream << "POINT_DATA " << numPoints << std::endl;
        outStream << "SCALARS ErrorValue float " << components << std::endl;
        outStream << "LOOKUP_TABLE default" << std::endl;

        // Write Data
        for (int i = 0; i < numPoints; i++)
        {
            outStream << scalarData[i] << std::endl;
        }

        // Close File
        outStream.close();

        return 1;
    }

    bool save_vtk(const int* scalarData, const char* path, const char* vtk_title, int components, int nx, int ny, int nz, int sx = 1, int sy = 1, int sz = 1) {
        // Open File
        std::ofstream outStream(path);
        if (outStream.fail()) return 0;

        int numPoints = nx * ny * nz * components;

        // Writing File Header
        outStream << "# vtk DataFile Version 4.0\n" << vtk_title << "\nASCII\nDATASET STRUCTURED_POINTS" << std::endl;
        outStream << "DIMENSIONS " << nx << " " << ny << " " << nz << " " << std::endl;
        outStream << "ORIGIN 0 0 0" << std::endl;
        outStream << "SPACING " << sx << " " << sy << " " << sz << std::endl;
        outStream << "POINT_DATA " << numPoints << std::endl;
        outStream << "SCALARS ErrorValue int " << components << std::endl;
        outStream << "LOOKUP_TABLE default" << std::endl;

        // Write Data
        for (int i = 0; i < numPoints; i++)
        {
            outStream << scalarData[i] << std::endl;
        }

        // Close File
        outStream.close();

        return 1;
    }

    bool save_vtk(const char* path, const char* vtk_title, const Texture& avg_dst, const Texture& channel_err, int sx = 1, int sy = 1, int sz = 1, bool binary = true) {
        vtk_points points;
        if (!avg_dst.data.empty()) {
            points.nx = avg_dst.dimensions.x;
            points.ny = avg_dst.dimensions.y;
            points.nz = avg_dst.dimensions.z;
        }
        points.sx = sx;
        points.sy = sy;
        points.sz = sz;
        points.title = vtk_title;

        std::string csv_path(path);
        csv_path = csv_path.replace(csv_path.find_last_of("."), csv_path.size(), ".csv");

        std::ofstream outCSV(csv_path);

        if (!outCSV.good()) {
            return 0;
        }

        std::vector<vtk_dataset<float>> errs;
        outCSV << "Distance RMS,Max Distance,Angular RMS,Max Angular,X RMS,Y RMS,Z RMS,Max X,Max Y,Max Z" << std::endl;
        std::array<std::string, 1> data_rows;
        data_rows[0] = "";

        if (!avg_dst.data.empty()) {
            errs.push_back({});
            errs.back().data = (float*)avg_dst.data.data();
            errs.back().data_components = 1;
            errs.back().data_title = "Average_Distance";
            errs.back().data_type = "float";

            double avg = 0.0;
            double max = 0.0;

            for (auto idx = 0; idx < avg_dst.dimensions.x * avg_dst.dimensions.y * avg_dst.dimensions.z; idx++) {
                auto val = errs.back().data[idx];
                auto mse = (val * val) / (avg_dst.dimensions.x * avg_dst.dimensions.y * avg_dst.dimensions.z);
                avg += mse;
                max = std::fmax(val, max);
            }

            avg = std::sqrt(avg);
            data_rows[0] += std::to_string(avg) + "," + std::to_string(max) + ",";
        }
        else {
            data_rows[0] += ",,";
        }
        
        data_rows[0] += ",,";

        if (!channel_err.data.empty()) {
            errs.push_back({});
            errs.back().data = (float*)channel_err.data.data();
            errs.back().data_components = 3;
            errs.back().data_title = "Component_Distance";
            errs.back().data_type = "float";

            double avg_x = 0.0;
            double avg_y = 0.0;
            double avg_z = 0.0;
            double max_x = 0.0;
            double max_y = 0.0;
            double max_z = 0.0;

            for (auto pos = 0; pos < channel_err.dimensions.x * channel_err.dimensions.y * channel_err.dimensions.z; pos++) {
                auto idx = pos * channel_err.channels;
                auto val_x = errs.back().data[idx + 0];
                auto val_y = errs.back().data[idx + 1];
                auto val_z = errs.back().data[idx + 2];

                auto mse_x = (val_x * val_x) / (avg_dst.dimensions.x * avg_dst.dimensions.y * avg_dst.dimensions.z);
                auto mse_y = (val_y * val_y) / (avg_dst.dimensions.x * avg_dst.dimensions.y * avg_dst.dimensions.z);
                auto mse_z = (val_z * val_z) / (avg_dst.dimensions.x * avg_dst.dimensions.y * avg_dst.dimensions.z);

                avg_x += mse_x;
                avg_y += mse_y;
                avg_z += mse_z;

                max_x = std::fmax(val_x, max_x);
                max_y = std::fmax(val_y, max_y);
                max_z = std::fmax(val_z, max_z);
            }

            avg_x = std::sqrt(avg_x);
            avg_y = std::sqrt(avg_y);
            avg_z = std::sqrt(avg_z);

            data_rows[0] += std::to_string(avg_x) + "," + std::to_string(avg_y) + "," + std::to_string(avg_z) + ",";
            data_rows[0] += std::to_string(max_x) + "," + std::to_string(max_y) + "," + std::to_string(max_z) + ",";
        }
        else {
            data_rows[0] += ",,,,,,";
        }

        // Remove last comma
        data_rows[0].resize(data_rows[0].size() - 1);
        outCSV << data_rows[0] << std::endl;

        outCSV.close();

        return save_vtk(path, points, errs, binary);
    }


    bool save_vtk(const char* path, const char* vtk_title, const Texture& tex, int sx = 1, int sy = 1, int sz = 1, bool binary = true) {
        vtk_points points;
        points.nx = tex.dimensions.x;
        points.ny = tex.dimensions.y;
        points.nz = tex.dimensions.z * tex.dimensions.w;
        points.sx = 1;
        points.sy = 1;
        points.sz = 1;
        points.title = "Vectorfield";

        int numPoints = points.nx * points.ny * points.nz;

        // Open File
        std::ofstream outVTK;

        if (binary) {
            outVTK.open(path, std::ios::out | std::ios::binary);
        }
        else {
            outVTK.open(path);
        }

        if (outVTK.fail()) return 0;

        // ====================================================
        //                      HEADER
        // ====================================================

        outVTK << "# vtk DataFile Version 4.0\n"
            << points.title << "\n"
            << (binary ? "BINARY" : "ASCII") << "\n"
            << "DATASET STRUCTURED_POINTS" << "\n"
            << "DIMENSIONS " << points.nx << " " << points.ny << " " << points.nz << " " << "\n"
            << "ORIGIN 0 0 0" << "\n"
            << "SPACING " << points.sx << " " << points.sy << " " << points.sz << "\n";

        // ====================================================
        //     DATASET ATTRIBUTES (POINT DATA/VERTEX DATA)
        // ====================================================

        outVTK << "POINT_DATA " << numPoints << std::endl;
        outVTK << "VECTORS " << points.title << " " << "float" << std::endl;

        for (auto point_idx = 0; point_idx < numPoints; point_idx++) {
            for (auto component = 0; component < tex.channels; component++) {
                auto scalar = tex.data[point_idx * tex.channels + component];

                if (binary) {
                    SwapEnd(scalar);

                    outVTK.write(reinterpret_cast<char*>(&scalar), sizeof(float));
                }
                else {
                    // Write error
                    outVTK << scalar;

                    // If vector, check whether to write space or endline
                    if (component == (tex.channels - 1)) {
                        outVTK << std::endl;
                    }
                    else {
                        outVTK << " ";
                    }

                }
            }
        }

        return 1;
    }
}