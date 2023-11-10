import {BuildingIcon}         from "@derivean/ui";
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
import {BuildingShapeSchema}  from "../schema/BuildingShapeSchema";

export namespace BuildingUpsertForm {
    export type Props =
        Form.PropsEx<
            withBuildingMutation,
            BuildingShapeSchema,
            withBuildingMutation["schema"]["request"],
            withBuildingMutation["schema"]["response"]
        >
        & WithEntity.Schema.$<BuildingSchema>;
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
        schema={BuildingShapeSchema}
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
