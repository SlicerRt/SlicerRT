import os
import numpy as np
import slicer

def prepareCt(beamNode, temp_path):

    import pyRadPlan.io.matRad as matRadIO
    from pyRadPlan.ct import validate_ct

    # accessing nodes in Slicer
    parentPlanNode = beamNode.GetParentPlanNode()
    referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()

    ct = {
        'resolution':{
            'x': referenceVolumeNode.GetSpacing()[0],
            'y': referenceVolumeNode.GetSpacing()[1],
            'z': referenceVolumeNode.GetSpacing()[2]
            },
        'cubeDim': referenceVolumeNode.GetImageData().GetDimensions(), #Matlab stores everything as double, but this should probably be handled internally
        'numOfCtScen': 1,
        'cube_hu': np.transpose(slicer.util.arrayFromVolume(referenceVolumeNode), (1,2,0))
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

    ct = validate_ct(ct)

    #Saving the ct dictionary as a .mat file
    matRadIO.save(os.path.join(temp_path,'ct.mat'), {'ct': ct.to_matrad_dict()})

    return ct


def prepareCst(beamNode, ct, temp_path):

    import pyRadPlan.io.matRad as matRadIO
    from pyRadPlan.cst import StructureSet, validate_cst, create_voi

    parentPlanNode = beamNode.GetParentPlanNode()
    referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()
    node = parentPlanNode.GetSegmentationNode()
    segNode = node.GetSegmentation()

    vois = []

    for id in segNode.GetSegmentIDs():
        segmentArray = slicer.util.arrayFromSegmentBinaryLabelmap(node, id, referenceVolumeNode)
        segmentArray = np.uint8(segmentArray)
        
        voi_type = 'TARGET' if id==parentPlanNode.GetTargetSegmentID() else 'OAR'
        voi = create_voi(voi_type=voi_type, name=id, ct_image=ct, mask=segmentArray)
        vois.append(voi)

    cst = StructureSet(vois=[voi], ct_image=ct)
    cst = validate_cst(cst, ct=ct)

    #Saving the cst dictionary as a .mat file
    matRadIO.save(os.path.join(temp_path,'cst.mat'), {'cst': cst.to_matrad_list()})

    return cst


def preparePln(beamNode, temp_path):

    import pyRadPlan.io.matRad as matRadIO
    from pyRadPlan.plan import validate_pln

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

    pln = validate_pln(pln)

    #Saving the pln dictionary as a .mat file
    matRadIO.save(os.path.join(temp_path,'ct.mat'), {'pln': pln.to_matrad_dict()})

    return pln
