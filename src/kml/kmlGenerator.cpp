
#include "kmlGenerator.hpp"

namespace KmlGenerator {

    KmlGenerator::KmlGenerator(const SensorDataArr &sensorData,
            const InterpData &interpData,
            const IsoLines &isolines, 
            const IsoContours &isocontours) :
        KmlFile(kmlFolder+kmlFileName),
        sensorData(sensorData),
        interpData(interpData),
        isolines(isolines),
        isocontours(isocontours)
    {

        generateKmlFile();
    }

    KmlGenerator::~KmlGenerator() {
    }

    void KmlGenerator::generateKmlFile() {

        putKmlHeader();

        // Styles definition
        putStationStyle();
        putIsoLineStyles();
        putIsoContourStyles();

        // Initial camera position
        putInitialView();

        // Station placemarks
        putStations();

        const unsigned int nData = std::min(maxDataProcessed, sensorData.nMeasures);

        unsigned int k = 0;
        for(const auto &interpolator : interpData) {
            
            const std::string &interpolatorName = interpolator.first;

            putFolder(interpolatorName, "Interpolator "+std::to_string(k)+": "+interpolatorName, false, false);
       
            // Ground overlays folder
            putInterpolatedDataOverlays(interpolatorName, nData);
        
            // Ground overlays
            putScreenOverlays(interpolatorName, nData);

            // Isolines and contours
            putIsoLines(interpolatorName, nData);
            putIsoContours(interpolatorName, nData);

            endFolder();
        }
        
        
        putKmlFooter();
    }

    void KmlGenerator::putKmlHeader() {
        time_t t = time(0);
        std::tm *now = localtime(&t);

        startKml();
        skipLine();
        putComment("=====================================================================================================================");
        putComment("This file was generated automatically with real meteorological data and is part of the Ensimag visualization project.");
        putComment("=====================================================================================================================");
        skipLine();
        startDocument("Root");
        putName("Environemental contaminant viewer");
        putDescription("PM10 particles");
        putDate(*now, YYYY_MM_DD_hh_mm_ss);
        putAuthor("Jean-Baptiste Keck");
        putAuthor("Alexandre Ribard");
        skipLine();
        putVisibility(true);
        putOpen(true);
        skipLine();
    }

    void KmlGenerator::putKmlFooter() {
        endDocument();
        endKml();
    }

    void KmlGenerator::putStationStyle() {
        startStyle("StationStyle");
        putLabelStyle(ColorRGBA(0x00,0x00,0x00,0x00), NORMAL, 0.0f); //invisible labels
        putIconStyle("http://ukmobilereview.com/wp-content/uploads/2013/07/antenna-strength.png",
                Offset(), 0.25f, 0.0f); //custom icon
        endStyle();
        skipLine();
    }

    void KmlGenerator::putIsoLineStyles() {

        for (const auto &interpIsolines : isolines) {
            for(const auto &temporalIsolines : interpIsolines.second) {
                for(const auto &isoline : temporalIsolines) {
                    startStyle("IsoLine_" + isoline.lines.color.toHexString());
                    putLineStyle(isoline.lines.color, NORMAL, 2u);
                    endStyle();
                }
            }
        }
        skipLine();
    }

    void KmlGenerator::putIsoContourStyles() {
        for (const auto &interpIsocontours : isocontours) {
            for(const auto &temporalIsocontours : interpIsocontours.second) {
                for(const auto &isocontour : temporalIsocontours) {
                    startStyle("IsoContour_" + isocontour.color.toHexString());
                    putPolyStyle(isocontour.color, NORMAL, true, true);
                    putLineStyle(ColorRGBA::black, NORMAL, 1u);
                    endStyle();
                }
            }
        }

        skipLine();
    }

    void KmlGenerator::putInitialView() {
        putLookAt((sensorData.bbox.xmin+sensorData.bbox.xmax)/2.0, (0.75*sensorData.bbox.ymin+0.25*sensorData.bbox.ymax), 0.0, CLAMP_TO_GROUND, 250000.0, 30.0f, -20.0f);
        skipLine();
    }

    void KmlGenerator::putScreenOverlays(const std::string &interpolatorName, unsigned int nData) {
        putFolder("GUI", "Screen Overlays (" + interpolatorName + ")", false, true);
    
        for (unsigned int i = 0; i < nData; i++) {
            putScreenOverlay("Color scale " + std::to_string(i), "",
                    Offset(0.0f, 0.0f),
                    Offset(0.1f, 0.15f),
                    Offset(0.0f, 400, PIXELS),
                    Offset(),
                    0.0f, 3, 
                    screenOverlayFolder+screenOverlayPrefix+interpolatorName+"_"+std::to_string(i)+"."+screenOverlayImgExt);
        }
        endFolder();
        skipLine();
    }

    void KmlGenerator::putInterpolatedDataOverlays(const std::string &interpolatorName, unsigned int nData) {
        putFolder("Interpolation results", "Data Overlays ("+interpolatorName+")", false, true);
        for (unsigned int i = 0; i < nData; i++) {
            putGroundOverlay("Data"+std::to_string(i), 
                    0u, CLAMP_TO_GROUND, 
                    sensorData.bbox, 
                    0.0, 
                    groundOverlayFolder+groundOverlayPrefix+interpolatorName+"_"+std::to_string(i)+"."+groundOverlayImgExt);
        }
        endFolder();
        skipLine();
    }

    void KmlGenerator::putStations() {
        putFolder("Stations", "Station locations", false, true);
        for (unsigned int i = 0; i < sensorData.nStations; i++) {
            putPlaceMark(*sensorData.stationNames[i], sensorData.stationDescription(i,getCurrentIndentLevel()+2), "StationStyle", 
                    sensorData.x[i], sensorData.y[i], 0.0, CLAMP_TO_GROUND);
        }
        endFolder();
        skipLine();
    }

    void KmlGenerator::putIsoLines(const std::string &interpolatorName, unsigned int nData) {
        
        const std::vector<IsoLineList<double,4u,float>> &interpIsolines = isolines.at(interpolatorName);

        putFolder("Isolines", "Data isolines (" + interpolatorName + ")", false, true);
        
        for(const auto &temporalIsolines : interpIsolines) {
            unsigned int i = 1;
            for(const auto &isoline : temporalIsolines) {
                putColorLineStrings("Isolines level " + std::to_string(i++), 
                        "Isovalue: " + std::to_string(isoline.value), 
                        "IsoLine_", 
                        isoline.lines);
            }
        }

        endFolder();
        skipLine();
    }

    void KmlGenerator::putIsoContours(const std::string &interpolatorName, unsigned int nData) {
        
        const std::vector<IsoContourList<double,4u,float>> &interpIsocontours = isocontours.at(interpolatorName);

        putFolder("Isocontours", "Data isocontours (" + interpolatorName + ")", false, false);
        
        for(const auto &temporalIsocontours : interpIsocontours) {
            unsigned int i = 1;
            for(const auto &isocontour : temporalIsocontours) {
                putColorPolygons("Isocontour level " + std::to_string(i++), 
                        "Isovalue: " + std::to_string(isocontour.value), 
                        "IsoContour_", 
                        ColorMultiLine<double,4u>(isocontour.lines, isocontour.color));
            }
        }

        endFolder();
        skipLine();
    }

}
