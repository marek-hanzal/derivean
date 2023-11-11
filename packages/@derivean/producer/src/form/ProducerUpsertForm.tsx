import {ProducerIcon}    from "@derivean/ui";
import {withDullSchema}  from "@use-pico/dull-stuff";
import {
    Form,
    NumberInput,
    TextInput
}                        from "@use-pico/form";
import {
    t,
    tx
}                        from "@use-pico/i18n";
import {type WithEntity} from "@use-pico/types";
import {type FC}         from "react";
import {ProducerRpc}     from "../rpc/ProducerRpc";
import {ProducerSchema}  from "../schema/ProducerSchema";

export namespace ProducerUpsertForm {
    export type Props =
        Form.PropsEx<
            ProducerRpc["mutation"],
            withDullSchema.Infer.ShapeSchema<ProducerSchema>,
            ProducerRpc["mutation"]["schema"]["request"],
            ProducerRpc["mutation"]["schema"]["response"]
        >
        & WithEntity.$<withDullSchema.Infer.Entity<ProducerSchema>>;
}

export const ProducerUpsertForm: FC<ProducerUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <Form
        text={{
            submit:  entity ? t()`Update producer (label)` : t()`Create producer (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Producer updated` : t()`Producer created`,
            }
        }}
        icon={<ProducerIcon/>}
        withMutation={ProducerRpc.mutation}
        schema={ProducerSchema.shape}
        inputs={{
            name: props => <TextInput
                label={t()`Producer name`}
                placeholder={tx()`Producer name (placeholder)`}
                {...props}
            />,
            time: props => <NumberInput
                label={t()`Production time`}
                {...props}
            />,
        }}
        values={entity}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            name: "",
            time: 0,
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
            <Input name={"time"}/>
        </>}
        {...props}
    />;
};
