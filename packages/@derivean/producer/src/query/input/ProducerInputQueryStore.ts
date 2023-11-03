"use client";

import {createQueryStore}         from "@use-pico/query";
import {ProducerInputQuerySchema} from "../../schema/input/ProducerInputQuerySchema";

export const ProducerInputQueryStore = createQueryStore({
    schema: ProducerInputQuerySchema,
});
