"use server";

import {ResourceQuerySchema} from "../schema/ResourceQuerySchema";
import {ResourceSchema}      from "../schema/ResourceSchema";

export const withResourceQueryAction = async (query: ResourceQuerySchema.Type): Promise<ResourceSchema.Type[]> => {
    return [];
};
