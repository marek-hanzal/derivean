"use client";

import {withDullUI}         from "@use-pico/dull-stuff";
import {ProducerRpc}        from "../rpc/ProducerRpc";
import {ProducerQueryStore} from "../store/ProducerQueryStore";

export const ProducerUI = withDullUI({
    rpc:        ProducerRpc,
    queryStore: ProducerQueryStore,
});
