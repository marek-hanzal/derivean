import {parse$}              from "@use-pico2/schema";
import {ErrorResponseSchema} from "../schema/ErrorResponseSchema";

export const isError = (input: any): input is ErrorResponseSchema.Type => {
    return parse$(ErrorResponseSchema, input).success;
};
