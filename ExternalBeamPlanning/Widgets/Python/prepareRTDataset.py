import os
import numpy as np
import slicer
from pyRadPlan.ct import validate_ct


def prepareCt(beamNode, temp_path):

    import pyRadPlan.io.matRad as matRadIO

    # accessing nodes in Slicer
    parentPlanNode = beamNode.GetParentPlanNode()
    referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()

    # MATLAB format (needed at the moment to run Matlab functions)
    cubeHU = np.empty((1, 1), dtype=object)
    cubeHU[0,0] = np.double(np.transpose(slicer.util.arrayFromVolume(referenceVolumeNode),(1,2,0)))



    ct = {
        'resolution':{
            'x': referenceVolumeNode.GetSpacing()[0],
            'y': referenceVolumeNode.GetSpacing()[1],
            'z': referenceVolumeNode.GetSpacing()[2]
            },
        'cubeDim': referenceVolumeNode.GetImageData().GetDimensions(), #Matlab stores everything as double, but this should probably be handled internally
        'numOfCtScen': 1,
        'cubeHU': cubeHU
    }

    

    origin=referenceVolumeNode.GetOrigin()

    ijkToRASDirections = np.eye(3)
    referenceVolumeNode.GetIJKToRASDirections(ijkToRASDirections)
    origin = ijkToRASDirections @ origin

    
    ct['x'] =  ct['resolution']['x']*np.arange(0,ct['cubeDim'][1]).astype(np.double) - ct['resolution']['x']/2.0 + origin[0]
    ct['y'] =  ct['resolution']['y']*np.arange(0,ct['cubeDim'][0]).astype(np.double) - ct['resolution']['y']/2.0 + origin[1]
    ct['z'] =  ct['resolution']['z']*np.arange(0,ct['cubeDim'][2]).astype(np.double) - ct['resolution']['z']/2.0 + origin[2]

    #ct['SliceThickness'] = np.ones(ct['cubeDim'][2])*ct['resolution']['z'] # ???? data.SliceThickness = ct['resolution']['z']
    ct['number_of_voxels'] = np.prod(ct['cubeDim'])

    #Saving the ct dictionary as a .mat file
    matRadIO.save(os.path.join(temp_path,'ct.mat'), {'ct': ct})

    # convert ct back to PYTHON format
    ct['cubeHU'] = np.transpose(slicer.util.arrayFromVolume(referenceVolumeNode),(1,2,0))

    return ct


def prepareCst(beamNode, temp_path):

    import pyRadPlan.io.matRad as matRadIO

    parentPlanNode = beamNode.GetParentPlanNode()
    referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()
    node = parentPlanNode.GetSegmentationNode()
    segNode = node.GetSegmentation()

    cst ={}
    index = 0

    for id in segNode.GetSegmentIDs():
        
        print('Converting segmentation: ' + id)
        segmentArray = slicer.util.arrayFromSegmentBinaryLabelmap(node, id, referenceVolumeNode)#.swapaxes(0,1)
        segmentArray = np.transpose(segmentArray,(1,2,0))
        segmentArray = np.asarray(segmentArray).ravel(order='F')
        nonZeroIndices=np.where([segmentArray>0])[1]

        print('Number of voxels in Segmentation: ' + str(len(nonZeroIndices)))

        cst[id] = {
            'index': index,
            'raw_indices': nonZeroIndices,
            'prioritized_indices': nonZeroIndices,
            'resized_indices': nonZeroIndices, #same indices (raw,prioritized,resized); like in patientLoader
        }

        if id==parentPlanNode.GetTargetSegmentID():
            cst[id]['type']='TARGET'
            cst[id]['doseObjective'] = 'SquaredDeviation'
            cst[id]['doseConstraint'] = [50.0,1000.0]
            cst[id]['parameters'] = {'priority' : 1.0,'alphaX' : 0.1,'betaX' : 0.05}
        else:
            cst[id]['type']='OAR'
            cst[id]['doseObjective'] = 'SquaredOverdosing'
            cst[id]['doseConstraint'] = [25.0,300.0]
            cst[id]['parameters'] = {'priority' : 2.0,'alphaX' : 0.1,'betaX' : 0.05}

        currentSegmentation = segNode.GetSegment(id)
        
        cst[id]['parameters']['visibleColor'] = currentSegmentation.GetColor()
        cst[id]['parameters']['Visible'] = True

        index += 1


    # MATLAB format (needed at the moment to run Matlab functions) - should be handled internally by pyRadPlan
    cstForMat = np.empty((len(cst),6), dtype=object)
    

    index = 0
    for id in cst:
        cellArrayIndices = np.empty((1,1), dtype=object)
        cellArrayDose = np.empty((1,1), dtype=object)
        cellArrayDoseParameters = np.empty((1,1), dtype=object)
        # print('Converting segmentation: ' + id + ' into matRad/pyRadPlan cst format')
        cellArrayIndices[0][0] = np.double(cst[id]['raw_indices']).reshape(-1,1) #???

        # print('Number of voxels in Segmentation: ' + str(len(cellArrayIndices[0][0])))
        
        cellArrayDoseParameters[0][0] = cst[id]['doseConstraint'][0]
        cellArrayDose[0][0] = {'className' : 'DoseObjectives.matRad_'+cst[id]['doseObjective'],
                            'parameters' : cellArrayDoseParameters,
                            'penalty' : cst[id]['doseConstraint'][1]
                            }

        cstForMat[index][0] = cst[id]['index']
        cstForMat[index][1] = id
        cstForMat[index][2] = cst[id]['type']
        cstForMat[index][3] = cellArrayIndices
        cstForMat[index][4] = cst[id]['parameters']
        cstForMat[index][5] = cellArrayDose


        index += 1

    #Saving the cst dictionary as a .mat file
    matRadIO.save(os.path.join(temp_path, 'cst.mat'), {'cst': cstForMat})

    return cst


def preparePln(beamNode, temp_path):

    import pyRadPlan.io.matRad as matRadIO

    '''
    All values in the pln dictionary need to be floats. INCLUDING the numOfFractions.
    '''
    parentPlanNode = beamNode.GetParentPlanNode()
    referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()

    origin=referenceVolumeNode.GetOrigin()
    ijkToRASDirections = np.eye(3)
    referenceVolumeNode.GetIJKToRASDirections(ijkToRASDirections)
    origin = ijkToRASDirections @ origin

    isocenter = [0]*3
    parentPlanNode.GetIsocenterPosition(isocenter)
    isocenter = ijkToRASDirections @ np.array(isocenter) - np.array(origin)#  + np.array(referenceVolumeNode.GetSpacing())/2.0

    pln = {
        "radiationMode": 'photons',
        "machine": 'Generic',
        "numOfFractions": 30.0,
        "propStf": {

            # beam geometry settings
            "bixelWidth": 5.0,
            "gantryAngles": [beamNode.GetGantryAngle()], #"gantryAngles": [parentPlanNode.GetBeamByNumber(i).GetGantryAngle() for i in range(1,numOfBeams+1)],
            "couchAngles": [beamNode.GetCouchAngle()], #[parentPlanNode.GetBeamByNumber(i).GetCouchAngle() for i in range(1,numOfBeams+1)],
            "numOfBeams": 1.0,
            "isoCenter": [isocenter]
        },

        # dose calculation settings
        "propDoseCalc": {"doseGrid": {"resolution": {"x": referenceVolumeNode.GetSpacing()[0], # 3.0,
                                                    "y": referenceVolumeNode.GetSpacing()[1], # 3.0,
                                                    "z": referenceVolumeNode.GetSpacing()[2], # 3.0,
                                                    },
                                    },
                        },

        # optimization settings
        "propOpt": {"optimizer": 'IPOPT',
                    "bioOptimization": 'none',
                    "runDAO": False,
                    "runSequencing": True
                    },
    }

    #Saving the pln dictionary as a .mat file
    matRadIO.save(os.path.join(temp_path, 'pln.mat'), {'pln': pln})

    return pln
