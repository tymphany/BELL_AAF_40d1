/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      State machine to control peer mode (sniff or active)
*/

/*! \brief Mirror Profile Peer Mode States

    @startuml

    state ACTIVE : In active mode
    state ENTER_SNIFF : Entering sniff mode
    state SNIFF : In sniff mode
    state EXIT_SNIFF : Exiting sniff mode

    ACTIVE --> ENTER_SNIFF : Entering sniff mode
    ENTER_SNIFF --> SNIFF : mode change = lp_sniff
    SNIFF --> EXIT_SNIFF : Exiting sniff mode
    EXIT_SNIFF --> ACTIVE : mode change = lp_active

    @enduml
*/

