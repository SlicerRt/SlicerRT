import os
import numpy as np
import slicer
import SimpleITK as sitk
import sitkUtils

def prepareCt(beamNode):
    from pyRadPlan.ct import create_ct
    parentPlanNode = beamNode.GetParentPlanNode()
    referenceVolumeNode = parentPlanNode.GetReferenceVolumeNode()
    image = sitkUtils.PullVolumeFromSlicer(referenceVolumeNode)
    return create_ct(cube_hu=image)


def prepareCst(beamNode, ct):
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
    return validate_cst(cst, ct=ct)


def preparePln(beamNode):
    from pyRadPlan.plan import IonPlan, create_pln

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
        "radiationMode": ['photons','protons','carbon'][slicer.pyRadPlanEngine.scriptedEngine.integerParameter(beamNode, 'radiationMode')],
        "machine": ['Generic'][slicer.pyRadPlanEngine.scriptedEngine.integerParameter(beamNode, 'machine')],
        "numOfFractions": slicer.pyRadPlanEngine.scriptedEngine.doubleParameter(beamNode, 'numOfFractions'),
        "propStf": {
            # beam geometry settings
            "bixelWidth": 5.0,
            "gantryAngles": [beamNode.GetGantryAngle()],
            "couchAngles": [beamNode.GetCouchAngle()],
            "numOfBeams": 1.0,
            "isoCenter": [isocenter]
        },

        # dose calculation settings
        "propDoseCalc": {"doseGrid": {"resolution": {"x": referenceVolumeNode.GetSpacing()[0],
                                                    "y": referenceVolumeNode.GetSpacing()[1],
                                                    "z": referenceVolumeNode.GetSpacing()[2],
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
    return create_pln(pln)




