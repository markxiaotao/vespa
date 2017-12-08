// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.controller.maintenance;

import com.yahoo.config.application.api.ValidationId;
import com.yahoo.config.provision.Environment;
import com.yahoo.config.provision.RegionName;
import com.yahoo.config.provision.Zone;
import com.yahoo.vespa.hosted.controller.Application;
import com.yahoo.vespa.hosted.controller.api.integration.athenz.NToken;
import com.yahoo.vespa.hosted.controller.api.integration.dns.Record;
import com.yahoo.vespa.hosted.controller.api.integration.dns.RecordName;
import com.yahoo.vespa.hosted.controller.application.ApplicationPackage;
import com.yahoo.vespa.hosted.controller.deployment.ApplicationPackageBuilder;
import com.yahoo.vespa.hosted.controller.deployment.DeploymentTester;
import com.yahoo.vespa.hosted.controller.persistence.MockCuratorDb;
import org.junit.Ignore;
import org.junit.Test;

import java.time.Duration;
import java.util.Optional;

import static com.yahoo.vespa.hosted.controller.application.DeploymentJobs.JobType.component;
import static com.yahoo.vespa.hosted.controller.application.DeploymentJobs.JobType.systemTest;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * @author mpolden
 */
public class DnsMaintainerTest {

    @Test
    @Ignore // TODO: Enable once DnsMaintainer actually removes records
    public void removes_record_for_unassigned_rotation() {
        DeploymentTester tester = new DeploymentTester();
        Application application = tester.createApplication("app1", "tenant1", 1, 1L);

        ApplicationPackage applicationPackage = new ApplicationPackageBuilder()
                .environment(Environment.prod)
                .globalServiceId("foo")
                .region("us-west-1")
                .region("us-central-1")
                .build();

        // Deploy application
        tester.deployCompletely(application, applicationPackage);
        assertEquals(1, tester.controllerTester().nameService().records().size());
        Optional<Record> record = tester.controllerTester().nameService().findRecord(
                Record.Type.CNAME, RecordName.from("app1.tenant1.global.vespa.yahooapis.com")
        );
        assertTrue(record.isPresent());
        assertEquals("app1.tenant1.global.vespa.yahooapis.com", record.get().name().asString());
        assertEquals("rotation-fqdn-01.", record.get().value().asString());

        // Remove application
        applicationPackage = new ApplicationPackageBuilder()
                .environment(Environment.prod)
                .allow(ValidationId.deploymentRemoval)
                .build();
        tester.notifyJobCompletion(component, application, true);
        tester.deployAndNotify(application, applicationPackage, true, systemTest);
        tester.applications().deactivate(application, new Zone(Environment.test, RegionName.from("us-east-1")));
        tester.applications().deactivate(application, new Zone(Environment.staging, RegionName.from("us-east-3")));
        tester.applications().deleteApplication(application.id(), Optional.of(new NToken("ntoken")));

        // DnsMaintainer removes record
        DnsMaintainer dnsMaintainer = new DnsMaintainer(tester.controller(), Duration.ofHours(12),
                                                     new JobControl(new MockCuratorDb()),
                                                     tester.controllerTester().nameService());
        dnsMaintainer.maintain();
        assertFalse("DNS record removed", tester.controllerTester().nameService().findRecord(
                Record.Type.CNAME, RecordName.from("app1.tenant1.global.vespa.yahooapis.com")).isPresent());
    }

}
