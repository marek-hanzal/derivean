"use client";

import {createQueryStore} from "@use-pico/client";
import {KingdomSchema}    from "../schema/KingdomSchema";

export const KingdomQueryStore = createQueryStore({
    schema: KingdomSchema.query,
});
export const {
    Provider: KingdomQueryProvider,
} = KingdomQueryStore;
