"use server";

import {container}              from "@derivean/server";
import {schema}                 from "@use-pico2/schema";
import {withAction}             from "@use-pico2/server";
import {withResourceRepository} from "../container/withResourceRepository";
import {ResourceQuerySchema}    from "../schema/ResourceQuerySchema";
import {ResourceSchema}         from "../schema/ResourceSchema";

export const withResourceQueryAction = withAction(container => ({
    request:  ResourceQuerySchema,
    response: schema(z => z.array(ResourceSchema)),
    action: async query => withResourceRepository.use(container).query(query),
}));

withResourceQueryAction(container)();
