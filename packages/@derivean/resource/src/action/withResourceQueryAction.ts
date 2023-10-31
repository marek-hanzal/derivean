"use server";

import {schema}                 from "@use-pico/schema";
import {withAction}             from "@use-pico/server";
import {withResourceRepository} from "../container/withResourceRepository";
import {ResourceQuerySchema}    from "../schema/ResourceQuerySchema";
import {ResourceSchema}         from "../schema/ResourceSchema";

export const withResourceQueryAction = withAction(container => ({
    request:  ResourceQuerySchema,
    response: schema(z => z.array(ResourceSchema)),
    action: async query => withResourceRepository.use(container).query(query),
}));
