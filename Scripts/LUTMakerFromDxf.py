import ezdxf
from ezdxf import units
from scipy.spatial import KDTree
import matplotlib.pyplot as plt
import numpy as np

class Hole:
    qDistance = 2500/300
    
    def __init__(self, coords):
        self.coords = coords
        q = Hole.quantizeHoleCoordinates(coords, Hole.qDistance)
        self.qCoords = q[0]
        self.qRowIdx = q[1][0]
        self.qColumnIdx = q[1][1]

    def __str__(self):
        return f'Hole(coords:{str(self.coords)}, qCoords:{str(self.qCoords)}, qIdx: Row#{self.qRowIdx}, Col#{self.qColumnIdx})'

    @staticmethod
    def quantizeHoleCoordinates(holeCoords, distance):
        qColIdx = round(holeCoords[0]/distance)
        qRowIdx = round(holeCoords[1]/distance)
        xq = round(holeCoords[0]/distance) * distance
        yq = round(holeCoords[1]/distance) * distance
        return ((xq, yq), (qRowIdx, qColIdx))



# holes is een array van Hole-klasse instanties, Startpoint is startPunt van de string,
# maxdistance is de max afstand naar het volgende gat
def makeHoleString(holes, startPoint, maxDistance):
    holeString = []
    holeCoords = np.array([h.coords for h in holes])
    # maak de KDTree vna alle gaten
    tree = KDTree(holeCoords)
    # zoek het dichtste gat bij de start
    dist, idx = tree.query(startPoint, k=2)
    closestHoleToStart = (dist[0], idx[0])
    startHoleIdx = closestHoleToStart[1]
    print(f'[INFO] Dichtste gat bij get startpunt is het gat met index {startHoleIdx} {holes[startHoleIdx].coords}')
    holeString.append(holes[startHoleIdx])

    # vul prev hole in om het zaadje van de riching waarin we willen werken te planten
    prevHoleIdx = startHoleIdx
    dist, idx = tree.query(holes[startHoleIdx].coords, k=2)
    currentHoleIdx = idx[1] # want 0 is de index van het gat zelf
    #print(currentHoleIdx)
    nextHoleDistance = 0
    
    while nextHoleDistance < maxDistance:
        holeString.append(holes[currentHoleIdx])
        # zoek de 2 dichtsbijzijnde gaten bij currentHole
        dist, idx = tree.query(holes[currentHoleIdx].coords, k=3)
        #print(idx)
            
        # check of we nog niet rond zijn
        if ((idx[1] != startHoleIdx) and (idx[2] != startHoleIdx)) or (prevHoleIdx == startHoleIdx):
            # een van de gevonden gaten is het vorige gat deze willen we niet
            if idx[1] != prevHoleIdx:
                # dan is idx[1] onze man
                nextHoleIdx = idx[1]
                nextHoleDistance = dist[1]
            else:
                # anders is idx[2] onze man
                nextHoleIdx = idx[2]
                nextHoleDistance = dist[2]
            #print(f'[INFO] Volgend gat wordt {nextHoleIdx}')
        else:
            # we zijn rond, stop maar
            #print(f'[INFO] We zijn rond!')
            nextHoleDistance = maxDistance
                
        prevHoleIdx = currentHoleIdx
        currentHoleIdx = nextHoleIdx
        #print(f'[INFO] Hudig gat: {currentHoleIdx}, vorig gat: {prevHoleIdx}')

    print(f'[INFO] hole string gemaakt met {len(holeString)} gaten') 
    return holeString

def getCircleCoordinatesFromLayer(dxfDocument, layerName):
    coords = []
    modelSpace = dxfDocument.modelspace()
    for dxfEntity in modelSpace:
        if (dxfEntity.dxftype() == 'CIRCLE') and (dxfEntity.dxf.layer == layerName):
##            xc = dxfEntity.dxf.center.x
##            yc = dxfEntity.dxf.center.y
##            coords.append((xc, yc))
            coords.append(dxfEntity.dxf.center)
    return coords


def writeCLookUpTable(holeString, xOffset, valuesPerLine = 20):
    tableSize = len(holeString)
    #npQHoleCoords = np.array([h.qCoords for h in holeString])
    with open("pixelLUT.c", "w") as f:
        f.write('#include "pixelLUT.h"\n\n')
        #f.write(f"#define TABLESIZE {tableSize}\n\n")
        f.write(f"const int pixelLookupTable[TABLESIZE][2] = {{\n")
        # 'valuesPerLine' waarden per lijn (dus 'valuesPerLine' paren)
        for i in range(0, tableSize, valuesPerLine):
            line = holeString[i:i+valuesPerLine]
            #lineStr = ", ".join(f"{{{sh.qColumnIdx}, {sh.qRowIdx}}}" for sh in line)
            lineStr = ", ".join(f"{{{xOffset+sh.qColumnIdx}, {abs(sh.qRowIdx)}}}" for sh in line) ## rekening houdend met rare positionering
            f.write("    " + lineStr)
            if i + valuesPerLine < tableSize:
                f.write(",\n")
            else:
                f.write("\n")
        f.write("};\n")

    hContent = f"""#ifndef PIXELLUT_H
#define PIXELLUT_H

#define TABLESIZE {tableSize}

extern const int pixelLookupTable[TABLESIZE][2];

#endif // PIXELLUT_H
"""
    with open("pixelLUT.h", "w") as f:
        f.write(hContent)

    print('[INFO] Look up table files geschreven (.c en .h). Kopieer beide files naar de directory D:/Users/Twenty Three BVBA/Documents/C_C++/PRUPixels') 

#############################  MAIN  ########################"
mijnDxf = ezdxf.readfile('bLAUWPOORTE LOGO-Layers.dxf')
#print(mijnDxf.units)
mijnDxf.units = units.IN
for layer in mijnDxf.layers:
    print(f"[INFO] Laag gevonden met naam: {layer.dxf.name}")


holeCoords = getCircleCoordinatesFromLayer(mijnDxf, "PIXELHOLES")
print(f"[INFO] aantal gaten gevonden: {len(holeCoords)}")

videoFieldWidth = 150

Hole.qDistance = 2500/videoFieldWidth
holes = []
for hc in holeCoords:
    hole = Hole(hc)
    holes.append(hole)



start1 = (-2310, -12, 0) # voor buitenrand schild
start2 = (-2041, -125, 0) # voor binnerand schild
start3 = (-1700, -470, 0) # voor buitenrand B
start4 = (-1463, -575, 0) # voor binnenrand boven B
start5 = (-1461, -1138, 0) # voor binnenrand onder B
holeString = makeHoleString(holes, start1, 40)
holeString.extend(makeHoleString(holes, start2, 40))
holeString.extend(makeHoleString(holes, start3, 40))
holeString.extend(makeHoleString(holes, start4, 40))
holeString.extend(makeHoleString(holes, start5, 40))
print(f"[INFO] lengte van de totale string: {len(holeString)}")
writeCLookUpTable(holeString, videoFieldWidth, 10)


# Maak de scatter plot
# Splits de x- en y-coördinaten (negeer z)
npHoleCoords = np.array([h.coords for h in holes])
x = npHoleCoords[:, 0] # selecteer kolom 0
y = npHoleCoords[:, 1] # selecteer kolom 1
plt.figure(figsize=(9, 9))
plt.scatter(x, y, c='blue', marker='o', alpha = 0.5)
npQHoleCoords = np.array([h.qCoords for h in holes])
qx = npQHoleCoords[:, 0]
qy = npQHoleCoords[:, 1]
plt.scatter(qx, qy, c='green', marker='o', alpha = 0.5)

npQHoleStringCoords = np.array([h.coords for h in holeString])
hx = npQHoleStringCoords[:, 0]
hy = npQHoleStringCoords[:, 1]
plt.scatter(hx, hy, c='red', marker='o', alpha = 0.5)

plt.scatter(start1[0], start1[1], marker='x', c='red', s=50, label="startpunt")

for i, (xi, yi, zi) in enumerate(npQHoleStringCoords):
    plt.text(xi + 0.5, yi + 0.5, str(i), fontsize=9, color="red")

#plt.plot([x[idx[0]], x[idx[1]]], [y[idx[0]], y[idx[1]]], c='red', linewidth=2, label="lijn tussen punt 0 en 1")

# Labels en titel
plt.xlabel('X-coördinaat')
plt.ylabel('Y-coördinaat')
plt.title('Weergaven van de LUT')
plt.grid(True)

# Plot tonen
plt.show()

