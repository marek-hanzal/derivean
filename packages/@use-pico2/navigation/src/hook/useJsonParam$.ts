import {
    parseJson$,
    type PicoSchema
}                  from "@use-pico2/schema";
import {useParam$} from "./useParam$";

export const useJsonParam$ = <
    TSchema extends PicoSchema,
>(schema: TSchema, name: string): PicoSchema.Output<TSchema> => {
    return parseJson$(schema, useParam$(name));
};
