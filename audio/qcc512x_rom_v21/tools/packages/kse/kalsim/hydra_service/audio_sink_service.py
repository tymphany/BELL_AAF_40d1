#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
'''
Hydra Audio Data Sink Service
'''

from .audio_data_service import HydraAudioDataService
from .constants import SERVICE_TYPE_AUDIO_DATA_SINK_SERVICE, DEVICE_TYPE_L2CAP


class HydraAudioSink(HydraAudioDataService):
    '''
    Hydra Audio Data Sink Service

    Args:
        hydra_protocol (kats.kalsim.hydra_service.protocol.HydraProcotol): Hydra messaging
            protocol instance
        device_type (int): Device type
        service_tag (int): Service tag when starting the service
    '''

    def __init__(self, hydra_protocol, device_type=DEVICE_TYPE_L2CAP, service_tag=1,
                 **kwargs):
        kwargs.setdefault('data_buffer_size', 1024)
        kwargs.setdefault('metadata_buffer_size', 0)
        kwargs.setdefault('kick_required', 1)
        kwargs.setdefault('metadata_header_length', 0)
        kwargs.setdefault('connect_endpoint', 0)
        kwargs.setdefault('space_handler', None)

        super(HydraAudioSink, self).__init__(
            hydra_protocol,
            SERVICE_TYPE_AUDIO_DATA_SINK_SERVICE,
            device_type,
            service_tag,
            **kwargs)
