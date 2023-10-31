"use server";

import {withAction}             from "@use-pico/server";
import {withResourceRepository} from "../container/withResourceRepository";
import {ResourceMutationSchema} from "../schema/ResourceMutationSchema";
import {ResourceSchema}         from "../schema/ResourceSchema";

// export const withMutationAction = (container => ({
//     request:  ResourceMutationSchema,
//     response: ResourceSchema,
//     table:    "",
//
// }));

export const withResourceMutationAction = withAction(container => ({
    request:  ResourceMutationSchema,
    response: ResourceSchema,
    action:   async mutation => withResourceRepository.use(container).mutate(mutation),
}));
