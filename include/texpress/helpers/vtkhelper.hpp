#pragma once
#include <fstream>
#include <texpress/types/texture.hpp>


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
		outVTK << "SCALARS " << err_ds.data_title << " " << err_ds.data_type << " " << err_ds.data_components << std::endl;
		outVTK << "LOOKUP_TABLE default" << std::endl;

		for (auto point_idx = 0; point_idx < numPoints; point_idx++) {
			auto error = err_ds.data[point_idx];

			if (binary) {
				SwapEnd(error);

				outVTK.write(reinterpret_cast<char*>(&error), sizeof(Terr));
			}
			else {
				outVTK << error << std::endl;
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

	bool save_vtk(const char* path, const char* vtk_title, const Texture& avg_dst, const Texture& avg_angular, int sx = 1, int sy = 1, int sz = 1, bool binary = true) {
		vtk_points points;
		if (!avg_dst.data.empty()) {
			points.nx = avg_dst.dimensions.x;
			points.ny = avg_dst.dimensions.y;
			points.nz = avg_dst.dimensions.z;
		}
		else {
			points.nx = avg_angular.dimensions.x;
			points.ny = avg_angular.dimensions.y;
			points.nz = avg_angular.dimensions.z;
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

		outCSV << "Average Distance,Max Distance,Average Angular Error,Max Angular Error" << std::endl;
		std::string data_row = "";

		std::vector<vtk_dataset<float>> errs;
		if (!avg_dst.data.empty()) {
			errs.push_back({});
			errs.back().data = (float*)avg_dst.data.data();
			errs.back().data_components = 1;
			errs.back().data_title = "Average_Distance";
			errs.back().data_type = "float";

			double avg = 0.0;
			double max = 0.0;
			long int counter = 0;
			for (auto idx = 0; idx < avg_dst.dimensions.x * avg_dst.dimensions.y * avg_dst.dimensions.z; idx++) {
				avg += errs.back().data[idx] / (avg_dst.dimensions.x * avg_dst.dimensions.y * avg_dst.dimensions.z);
				max = std::fmax(errs.back().data[idx], max);
			}

			data_row += std::to_string(avg) + "," + std::to_string(max) + ",";
		}

		if (!avg_angular.data.empty()) {
			errs.push_back({});
			errs.back().data = (float*)avg_angular.data.data();
			errs.back().data_components = 1;
			errs.back().data_title = "Average_Angular";
			errs.back().data_type = "float";

			double avg = 0.0;
			double max = 0.0;

			for (auto idx = 0; idx < avg_angular.dimensions.x * avg_angular.dimensions.y * avg_angular.dimensions.z; idx++) {
				avg += errs.back().data[idx] / (avg_angular.dimensions.x * avg_angular.dimensions.y * avg_angular.dimensions.z);
				max = std::fmax(errs.back().data[idx], max);
			}

			data_row += std::to_string(avg) + "," + std::to_string(max) + ",";
		}

		// Remove last comma
		data_row.resize(data_row.size() - 1);
		outCSV << data_row << std::endl;

		outCSV.close();

		return save_vtk(path, points, errs, binary);
	}
}