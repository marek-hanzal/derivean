"use server";

import {type ArraySchema}    from "@use-pico2/schema";
import {withAction}          from "@use-pico2/server";
import {ResourceQuerySchema} from "../schema/ResourceQuerySchema";
import {ResourceSchema}      from "../schema/ResourceSchema";

export const withResourceQueryAction = withAction<
    ResourceQuerySchema,
    ArraySchema<ResourceSchema>
>(async query => {
    return [
        {
            id:   "1234",
            name: "foo",
        },
        {
            id:   "12344",
            name: "bar",
        },
    ];
});
