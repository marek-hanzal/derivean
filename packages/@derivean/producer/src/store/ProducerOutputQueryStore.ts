"use client";

import {createQueryStore}     from "@use-pico/query";
import {ProducerOutputSchema} from "../schema/ProducerOutputSchema";

export const ProducerOutputQueryStore = createQueryStore({
    schema: ProducerOutputSchema.query,
});
