import {BuildingIcon}         from "@derivean/ui";
import {withDullSchema}       from "@use-pico/dull-stuff";
import {
    Form,
    TextInput
}                             from "@use-pico/form";
import {
    t,
    tx
}                             from "@use-pico/i18n";
import {type WithEntity}      from "@use-pico/types";
import {type FC}              from "react";
import {withBuildingMutation} from "../mutation/withBuildingMutation";
import {BuildingSchema}       from "../schema/BuildingSchema";

export namespace BuildingUpsertForm {
    export type Props =
        Form.PropsEx<
            withBuildingMutation,
            withDullSchema.Infer.ShapeSchema<BuildingSchema>,
            withBuildingMutation["schema"]["request"],
            withBuildingMutation["schema"]["response"]
        >
        & WithEntity.$<withDullSchema.Infer.Entity<BuildingSchema>>;
}

export const BuildingUpsertForm: FC<BuildingUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <Form
        text={{
            submit:  entity ? t()`Update building (label)` : t()`Create building (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Building updated` : t()`Building created`,
            }
        }}
        icon={<BuildingIcon/>}
        withMutation={withBuildingMutation}
        schema={BuildingSchema.shape}
        inputs={{
            name: props => <TextInput
                label={t()`Building name`}
                placeholder={tx()`Building name (placeholder)`}
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
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
        </>}
        {...props}
    />;
};
