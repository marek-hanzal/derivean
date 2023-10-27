import {type SourceSchema} from "@use-pico/source";

export interface IRepository<TSourceSchema extends SourceSchema> {
    readonly schema: TSourceSchema;
}
