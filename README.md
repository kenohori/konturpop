# konturpop

Simple script to estimate urban populations from the [Kontur population dataset](https://data.humdata.org/dataset/kontur-population-dataset).

The script first classifies [H3](https://h3geo.org) cells into urban and non-urban based on a population density threshold (default 5000/km^2). Urban cells are then aggregated as long as they form nearly contiguous regions (up to a maximum cell separation threshold). In this way, it is possible to generate city definitions in a standardised way throughout the world.

Links: [map](https://3d.bk.tudelft.nl/ken/maps/kontur-cities/) and [data](https://3d.bk.tudelft.nl/ken/maps/kontur-cities/cities.csv)