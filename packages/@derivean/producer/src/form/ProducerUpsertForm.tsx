import {ProducerIcon}         from "@derivean/ui";
import {
    Form,
    NumberInput,
    TextInput
}                             from "@use-pico/form";
import {type WithEntity}      from "@use-pico/types";
import {type FC}              from "react";
import {withProducerMutation} from "../mutation/withProducerMutation";
import {ProducerSchema}       from "../schema/ProducerSchema";
import {ProducerShapeSchema}  from "../schema/ProducerShapeSchema";

export namespace ProducerUpsertForm {
    export type Props =
        Form.PropsEx<
            withProducerMutation,
            ProducerShapeSchema,
            withProducerMutation["schema"]["request"],
            withProducerMutation["schema"]["response"]
        >
        & WithEntity.Schema.$<ProducerSchema>;
}

export const ProducerUpsertForm: FC<ProducerUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <Form
        icon={<ProducerIcon/>}
        withMutation={withProducerMutation}
        schema={ProducerShapeSchema}
        inputs={{
            name: TextInput,
            time: NumberInput,
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
