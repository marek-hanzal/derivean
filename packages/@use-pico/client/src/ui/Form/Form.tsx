import {
    type Infer,
    type Schema
}                       from "@use-pico/extras";
import {type ReactNode} from "react";

export namespace Form {
    export interface Props<
        TSchema extends Schema<any, any, any, any>,
    > {
        schema: TSchema;
        /**
         * Access to internal texting of the form
         */
        text?: {
            submit?: ReactNode;
            success?: {
                title?: ReactNode;
                message?: ReactNode;
            };
        };
        /**
         * Icon of the form (for example on "submit" button or so)
         */
        icon?: ReactNode;
        /**
         * Current values of the form, for example, data from API or so
         */
        values?: Infer.Shape<TSchema>;
        /**
         * Default values; not in terms of "current default" from entity, API, but *real* defaults.
         */
        defaultValues?: Infer.Shape<TSchema>;
    }
}

export const Form = <
    TSchema extends Schema<any, any, any, any>,
>(
    {
        schema,
        text,
        icon,
        values,
    }: Form.Props<TSchema>,
) => {
    return <div>
        bla!
    </div>;
};
