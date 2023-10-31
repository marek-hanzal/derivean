"use server";

import {CountSchema}            from "@use-pico/query";
import {withAction}             from "@use-pico/server";
import {withResourceRepository} from "../container/withResourceRepository";
import {ResourceQuerySchema}    from "../schema/ResourceQuerySchema";

export const withResourceCountAction = withAction(container => ({
    request:  ResourceQuerySchema,
    response: CountSchema,
    action:   async query => withResourceRepository.use(container).count(query),
}));
