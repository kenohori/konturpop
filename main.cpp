#include <iostream>
#include <fstream>

#include <unordered_map>
#include <unordered_set>
#include <list>

#include <ogrsf_frmts.h>
#include <h3/h3api.h>

struct City {
  H3Index id;
  std::unordered_set<H3Index> hexes;
  std::size_t population;
  double lat, lng;
};

int main(int argc, const char * argv[]) {
  
  const char *input_file = "/Users/ken/Downloads/kontur_population.gpkg";
  const std::size_t urban_threshold = 5000, city_threshold = 1000000;
  const int merge_distance = 2;
  
  int64_t max_hexes;
  maxGridDiskSize(merge_distance, &max_hexes);
  H3Index *hexes_within_distance = new H3Index[max_hexes];
  
  GDALAllRegister();
  GDALDataset *input_dataset = (GDALDataset*) GDALOpenEx(input_file, GDAL_OF_READONLY, NULL, NULL, NULL);
  if (input_dataset == NULL) {
    std::cerr << "Error: Could not open input file." << std::endl;
    return 1;
  }
  
  // Load data
  std::cout << "Loading data..." << std::endl;
  std::unordered_map<H3Index, std::size_t> population;
  std::unordered_map<H3Index, City> cities;
  for (auto &&input_layer: input_dataset->GetLayers()) {
    input_layer->ResetReading();
    OGRFeature *input_feature;
    while ((input_feature = input_layer->GetNextFeature()) != NULL) {
      for (int current_field = 0; current_field < input_feature->GetFieldCount(); ++current_field) {
        H3Index h3index;
        stringToH3(input_feature->GetFieldAsString("h3"), &h3index);
        double pop = input_feature->GetFieldAsDouble("population");
        population[h3index] = pop;
      }
    }
  } GDALClose(input_dataset);

  // Find urban hexes
  std::cout << "Finding urban hexes..." << std::endl;
  std::unordered_map<H3Index, H3Index> urban_hex_to_city;
  for (auto const &hex: population) {
    double area;
    cellAreaKm2(hex.first, &area);
    double density = population[hex.first]/area;
    if (density > urban_threshold) {
      urban_hex_to_city[hex.first] = hex.first;
    }
  }
  
  // Merge urban hexes into cities
  std::cout << "Merging urban hexes into cities..." << std::endl;
  for (auto const &hex_and_city: urban_hex_to_city) {
    
    if (hex_and_city.second != hex_and_city.first) continue;
    
    cities[hex_and_city.first].hexes.insert(hex_and_city.first);
    std::list<H3Index> hexes_to_check;
    hexes_to_check.push_back(hex_and_city.first);
    while (!hexes_to_check.empty()) {
      gridDisk(hexes_to_check.front(), merge_distance, hexes_within_distance);
      for (int i = 0; i < max_hexes; ++i) {
        if (urban_hex_to_city.count(hexes_within_distance[i]) &&
            cities[hex_and_city.first].hexes.count(hexes_within_distance[i]) == 0) {
          cities[hex_and_city.first].hexes.insert(hexes_within_distance[i]);
          hexes_to_check.push_back(hexes_within_distance[i]);
        }
      } hexes_to_check.pop_front();
    }
    
    std::size_t city_pop = 0;
    double lat_sum = 0.0, lng_sum = 0.0;
    for (auto const &city_hex: cities[hex_and_city.first].hexes) {
      urban_hex_to_city[city_hex] = hex_and_city.first;
      city_pop += population[city_hex];
      LatLng ll;
      cellToLatLng(city_hex, &ll);
      lat_sum += ll.lat;
      lng_sum += ll.lng;
    } cities[hex_and_city.first].population = city_pop;
    cities[hex_and_city.first].lat = lat_sum/cities[hex_and_city.first].hexes.size();
    cities[hex_and_city.first].lng = lng_sum/cities[hex_and_city.first].hexes.size();
  }
  
  
  // Write data
  std::cout << "Writing data..." << std::endl;
  std::ofstream output_stream;
  output_stream.open("/Users/ken/Downloads/urban.csv");
  output_stream << "lat,lng,pop,density,city,citypop\n";
  std::cout << urban_hex_to_city.size() << " urban hexes found" << std::endl;
  for (auto const &hex_and_city: urban_hex_to_city) {
    if (cities[hex_and_city.second].population < city_threshold) continue;
    LatLng ll;
    cellToLatLng(hex_and_city.first, &ll);
    double area;
    cellAreaKm2(hex_and_city.first, &area);
    double density = population[hex_and_city.first]/area;
    output_stream << 180.0*ll.lat/M_PI << "," << 180.0*ll.lng/M_PI << "," << population[hex_and_city.first] << "," << density << "," << hex_and_city.second << "," << cities[hex_and_city.second].population << "\n";
  } output_stream.close();
  output_stream.open("/Users/ken/Downloads/cities.csv");
  output_stream << "lat,lng,pop\n";
  for (auto const &city: cities) {
    if (city.second.population < city_threshold) continue;
    output_stream << 180.0*city.second.lat/M_PI << "," << 180.0*city.second.lng/M_PI << "," << city.second.population << "\n";
  } output_stream.close();
  return 0;
}
